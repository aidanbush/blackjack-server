/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct 25, 17
 * File: server.c
 * Description: contains the main server loop and main packet response functions
 */

/* standard libraries */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>

/* system libraries */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

/* project includes */
#include "server.h"
#include "packet.h"
#include "game.h"
#include "msg.h"

#define BACKLOG 5

extern int verbosity;

extern game_rules rules;
extern game_s game;
extern userlist_s userlist;

extern msg_list msg_queue;
extern msg_ack_list msg_ack_queue;

//server file descriptor
int sfd;

//atomic variable for exiting the main game loop
volatile sig_atomic_t exit_server = 0;

/* sigint handler setting the exit value */
void sigint_handler(int par) {
    exit_server = 1;
}

/* creates the SIGINT interrupt handler */
int create_sigint_handler() {
    struct sigaction interrupt = {
            .sa_handler = &sigint_handler
    };

    int err = sigaction(SIGINT, &interrupt, NULL);
    if (err == -1) {
        perror("sigaction");
        return -1;
    }
    return 0;
}

/* initializes the server and returns the socket file descriptor or -1 of errors*/
static int init_server() {
    struct addrinfo *res, hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
        .ai_flags = AI_PASSIVE
    };
    int err, sfd;

    err = getaddrinfo(NULL, rules.port, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "gai_err: %s\n", gai_strerror(err));
        return -1;
    }

    struct addrinfo *cur;
    for (cur = res; cur != 0; cur = cur->ai_next) {
        //create socket
        sfd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (sfd == -1) {
            perror("socket");
            continue;
        }

        int val = 1;
        //socket options if needed
        err = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        if (err == -1) {
            perror("setsockopt");
            close(sfd);
            continue;
        }

        err = bind(sfd, cur->ai_addr, cur->ai_addrlen);
        if (err == -1) {
            perror("bind");
            close(sfd);
            continue;
        }
        break;
    }

    freeaddrinfo(res);

    if (cur == NULL) {
        fprintf(stderr, "failed to connect\n");
        close(sfd);
        return -1;
    }

    return sfd;
}

/* sends the broadcast packet */
static int send_request() {
    if (verbosity >= 4)
        fprintf(stderr, "sending broadcast update\n");
    //create packet
    uint8_t *packet = create_status_packet(game.cur_player + 1);//check that the correct player

    // send packet to all players
    for (int i = 0; i < game.max_players; i++) {
        if (game.players[i] != NULL) {
            if (sendto(sfd, packet, STATUS_LEN, 0,
                        (struct sockaddr *) &game.players[i]->sock,
                        sizeof(game.players[i]->sock)) != STATUS_LEN)//bail on fail
                if (verbosity >= 1)
                    fprintf(stderr, "could not send to %s\n", game.players[i]->nick);
        }
    }
    free(packet);
    set_resend_timer();
    return 1;
}

/* sends the state to the given destination */
static int send_state(struct sockaddr_storage *dest) {
    //create packet
    uint8_t *packet = create_status_packet(0);
    if (packet == NULL) {
        if (verbosity >= 1)
            fprintf(stderr, "packet creation failed\n");
        return -1;
    }
    //send packet
    sendto(sfd, packet, STATUS_LEN, 0, (struct sockaddr *) dest, sizeof(*dest));
    free(packet);
    return 1;
}

/* sends the given error message to the given destination returning 1 on success
 *  and -1 on failure */
static int send_error(uint8_t error_opcode, struct sockaddr_storage *dest, char *msgstr) {
    uint8_t packet[ERROR_LEN];
    //memset 0
    memset(&packet, 0, ERROR_LEN);
    packet[0] = OPCODE_ERROR;
    packet[1] = error_opcode;
    //copy str
    if (msgstr != NULL)
        strncpy((char *) &packet[2], msgstr, ERROR_MSG_LEN);

    int len = sendto(sfd, packet, ERROR_LEN, 0, (struct sockaddr *) dest, sizeof(*dest));
    if (len != ERROR_LEN) {
        perror("sendto");
        if (verbosity >= 1)
            fprintf(stderr, "ERROR SEND LEN got:%d\n", len);
        return -1;
    }

    return 1;
}

static int op_msg(uint8_t *packet, int len) {
    if (len != MESSAGE_LEN)
        return -1;
    //add to ring buffer
    add_msg(packet);
    return 1;
}

static int op_ack(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    if (len != ACK_LEN)
        return -1;

    //get player id
    int p = get_player_sock(recv_store);
    if (p == -1)
        return -1;

    //else apply ack
    recive_ack(packet, p);
    return 1;
}

/* kicks or removes the player that sent the quit message */
static int op_quit(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    if (verbosity >= 2)
        fprintf(stderr, "received quit\n");

    //get player and check they exist
    int p = get_player_sock(recv_store);
    if (p == -1) {// check if player is in the game
        if (verbosity >= 1)
            fprintf(stderr, "quit from player not in game\n");
        return -1;
    }

    //set them to kicked
    kick_player(p);
    if (verbosity >= 3)
        fprintf(stderr, "kicked player:%d\n", p);
    //if state is idle or betting, and they have not bet delete them
    if (game.state == STATE_IDLE || (game.state == STATE_BET && game.players[p]->bet == 0)){
        delete_player(p);//delete them
    }
    // deal with current player
    if (p == game.cur_player) {
        next_player(game.cur_player);
        set_timer();
    }
    //if cur_player == -1 and there are no active players got to idle state
    if (game.state == STATE_BET && num_players() == 0) {
        game.cur_player = -1;
        game.state = STATE_IDLE;
        if (verbosity >= 3)
            fprintf(stderr, "state now STATE_IDLE\n");
    }

    //if state == STATE_PLAY and current player not
    if (game.state == STATE_PLAY && game.cur_player == -1) {
        game.state = STATE_FINISH;
        if (verbosity >= 3)
            fprintf(stderr, "state now STATE_FINISH\n");
    }

    //deal with messages
    return 1;
}

/* hits for the player that send a hit packet if it is a valid request */
static int op_hit(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    if (verbosity >= 2)
        fprintf(stderr, "received hit\n");

    int p = check_packet(packet, len, recv_store, HIT_LEN, STATE_PLAY);
    switch (p) {
        case P_CHECK_LEN:
            if (verbosity >= 1)
                fprintf(stderr, "incorrect HIT_LEN%d\n", len);
            return -1;
        case P_CHECK_STATE:
            if (verbosity >= 1)
                fprintf(stderr, "incorrect state not play\n");
            send_error(ERROR_OP_GEN, &recv_store, "not your turn to hit");
            return -1;
        case P_CHECK_DNE:
            if (verbosity >= 1)
                fprintf(stderr, "stand from player not in game\n");
            return -1;
        case P_CHECK_N_ACTIVE:
            if (verbosity >= 1)
                fprintf(stderr, "stand from non active player\n");
            return -1;
        case P_CHECK_N_CUR:
            if (verbosity >= 1)
                fprintf(stderr, "stand from non current player:%d\n", game.cur_player);
            send_error(ERROR_OP_GEN, &recv_store, "not your turn to hit");
            return -1;
        case P_CHECK_INVAL:
            if (verbosity >= 1)
                fprintf(stderr, "packet does not match the expected state\n");
            return -1;
        default:
            break;
    }

    //make hit play
    if (hit(p) == -1) {
        if (verbosity >= 3)
            fprintf(stderr, "player bust\n");
        //if bust set next player
        next_player(game.cur_player);
        set_timer();
        //if cur_layer == -1 next state
        if (game.cur_player == -1) {
            if (verbosity >= 3)
                fprintf(stderr, "state now STATE_FINISH, last player busted\n");
            game.state = STATE_FINISH;
        }
    }
    set_timer();
    send_request();
    return 1;
}

/* stands for the player that send a stand packet if it is a valid request */
static int op_stand(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
   if (verbosity >= 1)
        fprintf(stderr, "received stand\n");

    int p = check_packet(packet, len, recv_store, STAND_LEN, STATE_PLAY);

    switch (p) {
        case P_CHECK_LEN:
            if (verbosity >= 1)
                fprintf(stderr, "incorrect STAND_LEN%d\n", len);
            return -1;
        case P_CHECK_STATE:
            if (verbosity >= 1)
                fprintf(stderr, "incorrect state not play\n");
            send_error(ERROR_OP_GEN, &recv_store, "not your turn to stand");
            return -1;
        case P_CHECK_DNE:
            if (verbosity >= 1)
                fprintf(stderr, "stand from player not in game\n");
            return -1;
        case P_CHECK_N_ACTIVE:
            if (verbosity >= 1)
                fprintf(stderr, "stand from non active player\n");
            return -1;
        case P_CHECK_N_CUR:
            if (verbosity >= 1)
                fprintf(stderr, "stand from non current player:%d\n", game.cur_player);
            send_error(ERROR_OP_GEN, &recv_store, "not your turn to stand");
            return -1;
        case P_CHECK_INVAL:
            if (verbosity >= 1)
                fprintf(stderr, "packet does not match the expected state\n");
            return -1;
        default:
            break;
    }

    //set stand by updating current player
    next_player(game.cur_player);
    set_timer();
    //update state?
    if (game.cur_player == -1) {
        if (verbosity >= 3)
            fprintf(stderr, "state now STATE_FINISH, last player stood\n");
        game.state = STATE_FINISH;
        //deal with finish state
    }
    send_request();
    return 1;
}


/* apply the bet for the player that send a bet packet if it is a valid request */
static int op_bet(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    if (verbosity >= 1)
        fprintf(stderr, "received bet\n");

    int p = check_packet(packet, len, recv_store, BET_LEN, STATE_BET);
    switch (p) {
        case P_CHECK_LEN:
            if (verbosity >= 1)
                fprintf(stderr, "incorrect BET_LEN%d\n", len);
            return -1;
        case P_CHECK_STATE:
            if (verbosity >= 1)
                fprintf(stderr, "incorrect state not bet\n");
            send_error(ERROR_OP_GEN, &recv_store, "not your turn to bet");
            return -1;
        case P_CHECK_DNE:
            if (verbosity >= 1)
                fprintf(stderr, "bet from player not in game\n");
            return -1;
        case P_CHECK_N_ACTIVE:
            if (verbosity >= 1)
                fprintf(stderr, "bet from non active player\n");
            return -1;
        case P_CHECK_N_CUR:
            if (verbosity >= 1)
                fprintf(stderr, "bet from player non current player:%d\n", game.cur_player);
            send_error(ERROR_OP_GEN, &recv_store, "not your turn to bet");
            return -1;
        case P_CHECK_INVAL:
            if (verbosity >= 1)
                fprintf(stderr, "packet does not match the expected state\n");
            return -1;
        default:
            break;
    }
    //check if player already bet
    if (game.players[p]->bet != 0) {
        if (verbosity >= 1)
            fprintf(stderr, "bet from player who already bet\n");
        return -1;
    }

    //get bet
    uint32_t bet = get_bet(packet);
    //check if bet is valid and they have enough money
    if (bet < rules.min_bet || bet > game.players[p]->money) {
        if (verbosity >= 1)
            fprintf(stderr, "bet outside of valid range\n");
        send_error(ERROR_OP_MONEY, &recv_store, "bet outside of valid range");
        set_timer();
        return -1;
    }

    //set bet
    game.players[p]->bet = bet;

    //update current player
    next_player(game.cur_player);
    set_timer();

    //update game state
    if (game.cur_player == -1) {
        if (verbosity >= 3)
            fprintf(stderr, "state new STATE_PLAY, last player bet\n");
        deal_cards();//deal cards
        //set state
        game.state = STATE_PLAY;
    }
    //return success and update
    return 1;
}

/* connects a new player if there is room and the request is valid, then sends
 *  them the state */
static int op_connect(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    if (verbosity >= 1)
        fprintf(stderr, "GOT CONNECT\n");
    //if no space
    if (game.num_players >= game.max_players) {
        if (verbosity >= 1)
            fprintf(stderr, "send error SEATS\n");
        //send error message
        send_error(ERROR_OP_SEATS, &recv_store, "no more room");
        return -1;
    }

    // packet length incorrect
    if (len != CONNECT_LEN) {
        //send error message
        if (verbosity >= 1)
            fprintf(stderr, "invalid CONNECT_LEN%d\n", len);
        //return error
        return -1;
    }

    char *nick = get_connect_nick(packet);
    //copy nick into array on stack with null terminator;
    char new_nick[PLAYER_NAME_LEN + 1];
    if (strncpy(new_nick, nick, PLAYER_NAME_LEN) == NULL) {
        if (verbosity >= 1)
            fprintf(stderr, "send error strncpy fail\n");
        send_error(ERROR_OP_GEN, &recv_store, "unable to connect");
        return -1;
    }
    new_nick[PLAYER_NAME_LEN] = '\0';
    // verify nickname
    if (valid_nick(new_nick) == -1) {;
        //send error message
        if (verbosity >= 1)
            fprintf(stderr, "send error invalid nick\n");
        send_error(ERROR_OP_NICK_INV, &recv_store, "invalid nickname");
        return -1;
    }

    //add player
    int pos = add_player(nick, recv_store);
    if (pos == -1) {//need to also add connection information
        if (verbosity >= 1)
            fprintf(stderr, "send error add player fail\n");
        send_error(ERROR_OP_GEN, &recv_store, "unable to connect");
        return -1;//if error return error don't respond to save memory
    } else if (pos == -2) {
        if (verbosity >= 1)
            fprintf(stderr, "send error nick taken\n");
        send_error(ERROR_OP_NICK_TAKEN, &recv_store, "nickname already taken");
        return -1;
    }

    //if in idle state set them to be active or they are after the current betting player
    if (game.state == STATE_IDLE || (game.state == STATE_BET && game.cur_player < pos)) {//currently redundant
        game.players[pos]->active = PLAYER_A_ACTIVE;
    }

    if (game.cur_player == -1 && game.state == STATE_IDLE) {//if the current player is -1 and the state is idle
        next_player(game.cur_player);
        set_timer();
    }

    //if idle go into bet
    if (game.state == STATE_IDLE) {
        if (verbosity >= 1)
            fprintf(stderr, "state now STATE_BET, first player joined\n");
        game.state = STATE_BET;
    }

    if (verbosity >= 4)
        fprintf(stderr, "sending connect state update\n");
    send_state(&recv_store);
    return 1;//success
}

/* prints out the given card */
static void print_card(uint8_t card) {
    int v = ((card - 1) % 13) + 1;
    switch (v) {
        case 13:
            fprintf(stdout, " K");
            break;
        case 12:
            fprintf(stdout, " Q");
            break;
        case 11:
            fprintf(stdout, " J");
            break;
        case 10:
            fprintf(stdout, " T");
            break;
        case 1:
            fprintf(stdout, " A");
            break;
        default:
            fprintf(stdout, " %d", v);
            break;
    }
}

/* prints out a given player hand and information*/
static void print_player(int p) {
    player_s *player = game.players[p];
    if (player == NULL) {
        fprintf(stdout, "player[%d] bnk: %d bet: %d active: %d\n", p, 0, 0, 0);
        fprintf(stdout, " hand: none\n");
    } else {
        fprintf(stdout, "player[%d] bnk: %d bet: %d active: %d\n", p, player->money, player->bet, player->active);
        fprintf(stdout, " hand:");
        for (int i = 0; i < player->num_cards; i++) {
            uint8_t card = player->cards[i];
            print_card(card);
        }
        fprintf(stdout, "\n");
    }
}

/* prints out the current game state */
static void print_state() {
    fprintf(stdout, "\n");
    //print state
    switch (game.state) {
        case STATE_IDLE:
            fprintf(stdout, "STATE_IDLE\n");
            break;
        case STATE_BET:
            fprintf(stdout, "STATE_BET\n");
            break;
        case STATE_PLAY:
            fprintf(stdout, "STATE_PLAY\n");
            break;
        case STATE_FINISH:
            fprintf(stdout, "STATE_FINISH\n");
            break;
        default:
            fprintf(stdout, "ERROR IN STATE\n");
            break;
    }

    fprintf(stdout, "num players: %d\n", num_players());
    fprintf(stdout, "current player: %d\n", game.cur_player);

    //print dealer
    fprintf(stdout, "dealer\n hand:");
    for (int i = 0; i < game.d_num_cards; i++)
        print_card(game.d_cards[i]);
    fprintf(stdout, "\n");

    //print players
    fprintf(stdout, "players\n");
    for (int i = 0; i < game.max_players; i++)
        print_player(i);
    fprintf(stdout, "\n");
}

/* sets up that game to start a new round */
static void start_new_round() {
    set_players_active();
    if (verbosity >= 1)
        fprintf(stderr, "state now STATE_BET, new round starting\n");
    game.state = STATE_BET;
    //set player
    next_player(-1);//deal with being kicked or not active
    set_timer();
    //send request to player
    send_request();
}

/* NOT TESTED */
/* sends the msg and msg_pos to all the connected players, unless they have acked it*/
static void send_msg(int msg_pos) {
    //for all players
    for (int i = 0; i < game.max_players; i++)
        //if not null
        if (game.players[i] != NULL)
            //if not acked
            if (!msg_acked(msg_pos, i))
                sendto(sfd, msg_queue.msgs[msg_pos], MESSAGE_LEN, 0,
                        (struct sockaddr *) &game.players[i]->sock,
                        sizeof(game.players[i]->sock));
}

/* NOT TESTED */
/* sends all the messages to all players who have not acked them */
static void send_msgs() {
#ifndef _TEST_MSGS//macro to not try to forward messages for testing purposes
    return;
#endif /* _TEST_MSGS */
    if (verbosity >= 4)
        fprintf(stderr, "sending messages\n");
    //for all messages
    for (int i = 0; i < msg_queue.size; i++)
        //if msg not null
        if (msg_queue.msgs[i] != NULL)
            //send msg
            send_msg(i);
}

/* checks if the timers are up and deals with them if so */
static void check_timers() {
    //if there is a current player and the current player is not null
    if (game.cur_player != -1 && game.players[game.cur_player] != NULL) {
        struct sockaddr_storage cur_sock = game.players[game.cur_player]->sock;
        if(check_kick()) { // check kick
            send_error(ERROR_OP_TIME, &cur_sock, "");
            //if i just kicked the last player update the state
            if (game.cur_player == -1) {
                if (game.state == STATE_PLAY) {
                    if (verbosity >= 1)
                        fprintf(stderr, "state now STATE_FINISH, last player in play round timed out\n");
                    game.state = STATE_FINISH;
                }
                if (game.state == STATE_BET) {
                    if (verbosity >= 1)
                        fprintf(stderr, "state now STATE_PLAY, last player in bet round timed out\n");
                    game.state = STATE_PLAY;
                }
            }
        }
    }

    //check resend
    if (check_resend_timer() && game.state != STATE_IDLE) {
        if (verbosity >= 4)
            fprintf(stderr, "resending packet\n");

        send_msgs();

        //resend
        send_request();
        //if in final state check if i need to change state
        if (game.state == STATE_FINISH) {
            if (game.finish_resend >= FINISH_SEND_THRESHOLD) {
                if (verbosity >= 3)
                    fprintf(stderr, "state now STATE_IDLE, end of finish state\n");
                //update state
                game.state = STATE_IDLE;//check state to go into
                //reset finish_resend
                game.finish_resend = 0;

                kick_bankrupt();
                for (int i = 0; i < game.max_players; i++) {
                    if (game.players[i] != NULL && game.players[i]->active == PLAYER_A_KICKED) {
                        if (verbosity >= 1)
                            fprintf(stderr, "deleting kicked player:%d\n", i);
                        //send error message if they are bankrupt
                        if (game.players[i]->money < rules.min_bet)
                            send_error(ERROR_OP_MONEY, &game.players[i]->sock, "");
                        //delete
                        delete_player(i);
                    }
                }

                reset_game();
                if (num_players() != 0) {
                    start_new_round();
                }
            } else {
                game.finish_resend++;
            }
        }
        //reset timer
        set_resend_timer();
    }
}

/* main server loop function */
void server() {
    uint8_t packet[MAX_PACKET_LEN];

    sfd = init_server();
    if (sfd == -1) {
        if (verbosity >= 1)
            fprintf(stderr, "unable to create server\n");
        return;
    }

    //setup interrupt
    if (create_sigint_handler() == -1) {
        if (verbosity >= 1)
            fprintf(stderr, "unable to setup interupt handler\n");
        return;
    }

    fd_set mfds, readfds;
    FD_ZERO(&mfds);
    FD_SET(sfd, &mfds);

    int nrdy, opcode;
    struct timeval timeout;
    struct sockaddr_storage recv_store;
    socklen_t recv_addr_len = sizeof(recv_store);
    int recv_len;

    // main loop
    while(!exit_server) {
        //select setup
        readfds = mfds;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100e3;

        nrdy = select(sfd + 1, &readfds, NULL, NULL, &timeout);

        // check error
        if (nrdy == -1) {
            //if not EINTER
            if (errno != EINTR)
                if (verbosity >= 1) perror("select");
            continue;
        }
        // timeout
        if (nrdy == 0) {
            goto end_of_checks;
            continue;
        }

        if (verbosity >= 4)
            fprintf(stderr, "recieved a packet\n");
        recv_len = recvfrom(sfd, &packet, sizeof(packet), 0,
                            (struct sockaddr*) &recv_store, &recv_addr_len);

        opcode = get_opcode(packet);

        if (verbosity >= 5) {
            fprintf(stderr, "print STATE before\n");
            print_state();
        }

        //switch on opcode
        switch (opcode) {
            //each opcode calls different function
            case OPCODE_CONNECT:
                op_connect(packet, recv_len, recv_store);
                break;
            case OPCODE_BET:
                op_bet(packet, recv_len, recv_store);
                break;
            case OPCODE_STAND:
                op_stand(packet, recv_len, recv_store);
                break;
            case OPCODE_HIT:
                op_hit(packet, recv_len, recv_store);
                break;
            case OPCODE_QUIT:
                op_quit(packet, recv_len, recv_store);
                break;
            case OPCODE_MESSAGE:
                op_msg(packet, recv_len);
                break;
            case OPCODE_ACK:
                op_ack(packet, recv_len, recv_store);
                break;
            default:
                break;
        }

end_of_checks:
        if (game.cur_player == -1 && game.state == STATE_PLAY) {//update for play state
            if (verbosity >= 3)
                fprintf(stderr, "play round start initializing first player\n");
            next_player(game.cur_player);
            set_timer();
            send_request();
        } else if (game.state == STATE_FINISH && game.finish_resend == 0) {//finished state and first time in it -- use game.finish_resend
            if (verbosity >= 3)
                fprintf(stderr, "final state start initializing end of game logic\n");
            //make dealers moves
            dealer_play();
            //update money
            round_end();
            send_request();
            game.finish_resend++;
        }
        //if i need to start the round from idle
        if (game.cur_player == -1 && game.state == STATE_IDLE && num_players() != 0) {
            start_new_round();//set players to be active
        }
        //check timers
        check_timers();
        //if it did not jump from the timeout
        if (nrdy != 0) if (verbosity >= 5) print_state();
    }

    close(sfd);
}
