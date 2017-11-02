/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct 25, 17
 * File:
 * Description:
 */

/* standard libraries */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

/* system libraries */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

/* project includes */
#include "packet.h"
#include "game.h"

#define BACKLOG 5
#define PORT "4420"

int sfd;

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

        //bind
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

static int send_request() {
    //create packet
    uint8_t *packet = create_status_packet(game.cur_player + 1);//check that the corect player

    // send packet to all players
    for (int i = 0; i < game.max_players; i++) {
        if (game.players[i] != NULL) {
            if (sendto(sfd, packet, STATUS_LEN, 0,
                        (struct sockaddr *) &game.players[i]->sock,
                        sizeof(game.players[i]->sock)) != STATUS_LEN)//check for EINTER otherwise bail
                fprintf(stderr, "could not send to %s\n", game.players[i]->nick);
        }
    }
    set_resend_timer();//--------------------------------resend timer
    return 1;
}

static int send_state(struct sockaddr_storage *dest) {
    //create packet
    uint8_t *packet = create_status_packet(0);
    if (packet == NULL) {
        fprintf(stderr, "packet creation failed\n");
        return -1;
    }
    //send packet
    sendto(sfd, packet, STATUS_LEN, 0, (struct sockaddr *) dest, sizeof(*dest));
    free(packet);
    return 1;
}

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
        fprintf(stderr, "ERROR SEND LEN got:%d\n", len);
        return -1;
    }

    return 1;
}

/*int ack(packet) {
    //if acked msg exists
        //remove acked msg from buffer
        //return success
    //else send error msg
        //return error
}*/

//need to deal with when im in the bet state and have only the one client
static int op_quit(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    fprintf(stderr, "recieved quit\n");
    //if incorrect length
        //still let quit ignore

    //get player and check they exist
    int p = get_player_sock(recv_store);
    if (p == -1) {// check if player is in the game
        fprintf(stderr, "quit from player not in game\n");
        return -1;
    }

    //set them to kicked
    kick_player(p);
    fprintf(stderr, "kicked player:%d\n", p);
    //if state is idle or beting, and they have not bet delete them
    if (game.state == STATE_IDLE || (game.state == STATE_BET && game.players[p]->bet == 0)){
        delete_player(p);//delete them
    }
    // deal with current player
    if (p == game.cur_player) {
        next_player(game.cur_player);
        set_timer();
        fprintf(stderr, "updated current player now:%d\n", game.cur_player);
    }
    //if cur_player == -1 and there are no active players got to idle state
    if (game.state == STATE_BET && num_players() == 0) {
        game.cur_player = -1;
        game.state = STATE_IDLE;
        fprintf(stderr, "no players\n");
    }

    //if state == STATE_PLAY and
    if (game.state == STATE_PLAY && game.cur_player == -1) {
        fprintf(stderr, "in play state, and last active player quit\n");
        game.state = STATE_FINISH;
    }

    //deal with messages
    return 1;
}

/*int msg(packet) {
    //check if in proper state
        //if not return failure
    //add to ring buffer
    //if failed return return error
    //return success
}*/

static int op_hit(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    fprintf(stderr, "recived hit\n");
    //check length
    if (len != HIT_LEN) {
        fprintf(stderr, "send error HIT_LEN%d\n", len);
        send_error(ERROR_OP_GEN, &recv_store, "");
        return -1;
    }

    //check state
    if (game.state != STATE_PLAY) {
        fprintf(stderr, "incorrect state not play\n");
        send_error(ERROR_OP_GEN, &recv_store, "");
        return -1;
    }

    //get player
    int p = get_player_sock(recv_store);
    if (p == -1) {// check if player is in the game
        fprintf(stderr, "stand from player not in game\n");
        return -1;
    }

    //if player not active ignore
    if (game.players[p]->active != 1) {
        fprintf(stderr, "stand from non active player\n");
        return -1;
    }

    //check if asking for that player TODO could remove, probably shouldn't
    if (p != game.cur_player) {
        fprintf(stderr, "stand from non current player:%d\n", game.cur_player);
        return -1;
    }

    //make hit play
    if (hit(p) == -1) {
        fprintf(stderr, "player bust\n");
        //if bust set next player
        next_player(game.cur_player);
        set_timer();
        //if cur_layer == -1 next state
        if (game.cur_player == -1) {
            fprintf(stderr, "last player busted, state = STATE_FINAL\n");
            game.state = STATE_FINISH;
        }
    }
    set_timer();
    return 1;
}

static int op_stand(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    fprintf(stderr, "recieved stand\n");
    //check length
    if (len != STAND_LEN) {
        fprintf(stderr, "send error STAND_LEN%d\n", len);
        send_error(ERROR_OP_GEN, &recv_store, "");
        return -1;
    }

    //check state
    if (game.state != STATE_PLAY) {
        fprintf(stderr, "incorrect state not play\n");
        send_error(ERROR_OP_GEN, &recv_store, "");
        return -1;
    }

    //get player
    int p = get_player_sock(recv_store);
    if (p == -1) {// check if player is in the game
        fprintf(stderr, "stand from player not in game\n");
        return -1;
    }

    //player not active ignore
    if (game.players[p]->active != 1) {
        fprintf(stderr, "stand from non active player\n");
        return -1;
    }
    //check if asking for that players play TODO could remove??? may not want to
    if (p != game.cur_player) {
        fprintf(stderr, "stand from non current player:%d\n", game.cur_player);
        return -1;
    }

    //set stand by updating current player
    next_player(game.cur_player);
    set_timer();
    //update state?
    if (game.cur_player == -1) {
        fprintf(stderr, "last players stand, state = STATE_FINAL\n");
        game.state = STATE_FINISH;
        //deal with finish state
    }
    return 1;
}


static int op_bet(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    fprintf(stderr, "recieved bet\n");
    //check packet length
    if (len != BET_LEN) {
        fprintf(stderr, "send error BET_LEN%d\n", len);
        send_error(ERROR_OP_GEN, &recv_store, "");
        return -1;
    }

    //check if in proper state
    if (game.state != STATE_BET) {
        fprintf(stderr, "incorrect state not bet\n");
        //send error
        send_error(ERROR_OP_GEN, &recv_store, "");
        //if not return failure
        return -1;
    }
    //get player
    int p = get_player_sock(recv_store);
    if (p == -1) { //player not in game
        fprintf(stderr, "bet from player not in game\n");
        return -1;
    }
    //if player not active ignore
    if (game.players[p]->active != 1) {
        fprintf(stderr, "bet from non active player\n");
        return -1;
    }
    //check if player already bet
    if (game.players[p]->bet != 0) {
        fprintf(stderr, "bet from player who already bet\n");
        return -1;
    }
    //check if asking for that players bet TODO: may want to remove
    if (p != game.cur_player) {
        fprintf(stderr, "bet from player non current player:%d\n", game.cur_player);
        return -1;
    }
    //get bet
    uint32_t bet = get_bet(packet);
    //check if bet is valid and they have enough money
    if (bet < rules.min_bet || bet > game.players[p]->money) {
        fprintf(stderr, "send error\n");
        send_error(ERROR_OP_MONEY, &recv_store, "");
        return -1;
    }

    //set bet
    game.players[p]->bet = bet;

    //update current player
    next_player(game.cur_player);
    set_timer();

    //update gamestate
    if (game.cur_player == -1) {
        fprintf(stderr, "update state\n");
        deal_cards();//deal cards
        //set state
        game.state = STATE_PLAY;
    }
    //return success and update
    return -1;
}

static int op_connect(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    fprintf(stderr, "GOT CONNECT\n");
    //if no space
    if (game.num_players >= game.max_players) {
        fprintf(stderr, "send error SEATS\n");
        //send error message
        send_error(ERROR_OP_SEATS, &recv_store, "");
        return -1;
    }

    // packet length incorrect
    if (len != CONNECT_LEN) {
        //send error message
        fprintf(stderr, "send error CONNECT_LEN%d\n", len);
        send_error(ERROR_OP_GEN, &recv_store, "");
        //return error
        return -1;
    }

    char *nick = get_connect_nick(packet);
    //copy nick into array on stack with null terminator;
    char new_nick[PLAYER_NAME_LEN + 1]; // TODO test---------------------
    if (strncpy(new_nick, nick, PLAYER_NAME_LEN) == NULL) {
        fprintf(stderr, "send error strncpy fail\n");
        send_error(ERROR_OP_GEN, &recv_store, "");
        return -1;
    }
    new_nick[PLAYER_NAME_LEN] = '\0';
    // verify nickname
    if (valid_nick(new_nick) == -1) {;
        //send error message
        fprintf(stderr, "send error invalid nick\n");
        send_error(ERROR_OP_NICK_INV, &recv_store, "");
        return -1;
    }

    //add player
    int pos = add_player(nick, recv_store);
    if (pos == -1) {//need to also add connection information
        fprintf(stderr, "send error add player fail\n");
        send_error(ERROR_OP_GEN, &recv_store, "");
        return -1;//if error return error dont respond to save memory???--------------
    } else if (pos == -2) {
        fprintf(stderr, "send error nick taken\n");
        send_error(ERROR_OP_NICK_TAKEN, &recv_store, "");
        return -1;
    }

    //if in idle state set them to be active or they are after the current betting player
    if (game.state == STATE_IDLE || (game.state == STATE_BET && game.cur_player < pos)) {//currently redundant
        game.players[pos]->active = 1;
    }

    if (game.cur_player == -1 && game.state == STATE_IDLE) {//if the current player is -1 and the state is idle
        next_player(game.cur_player);
        set_timer();
    }

    //if idle go into bet
    if (game.state == STATE_IDLE) {
        fprintf(stderr, "Switing to bet state\n");
        game.state = STATE_BET;
    }

    //broadcast update
    send_state(&recv_store);
    return 1;//success
}

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

static void print_state() {
    fprintf(stderr, "\n");
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
            fprintf(stdout, "ERROR\n");
            break;
    }

    fprintf(stderr, "num players: %d\n", num_players());
    fprintf(stderr, "current player: %d\n", game.cur_player);

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

static void check_timers() {
    check_kick();
    //check resend
    if (check_resend_timer() && game.state != STATE_IDLE) {
        fprintf(stderr, "resend timer\n");
        //resend
        //send_request();
        //if in final state check if i need to change state
        if (game.state == STATE_FINISH) {
            fprintf(stderr, "game state == FINISH\n");
            //increment counter
            game.finish_resend++;
            //if counter above threshold
            if (game.finish_resend >= FINISH_SEND_THRESHOLD) {
                fprintf(stderr, "ending finish state\n");
                //update state
                game.state = STATE_IDLE;
                //reset finish_resend
                game.finish_resend = 0;
            }
        }
        //reset timer
        set_resend_timer();
    }
}

void server() {
    uint8_t packet[MAX_PACKET_LEN];

    sfd = init_server();
    if (sfd == -1) {
        fprintf(stderr, "unable to create server\n");
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
    while(1/*interrupt atomic type variable*/) {
        //select setup
        readfds = mfds;
        timeout.tv_sec = 0;
        timeout.tv_usec = 250e3;

        nrdy = select(sfd + 1, &readfds, NULL, NULL, &timeout);

        // check error
        if (nrdy == -1) {
            fprintf(stderr, "ERROR\n");
            continue;
        }
        // timeout
        if (nrdy == 0) {
            check_timers();
            continue;
        }

        fprintf(stderr, "got msg\n");
        recv_len = recvfrom(sfd, &packet, sizeof(packet), 0,
                            (struct sockaddr*) &recv_store, &recv_addr_len);

        opcode = get_opcode(packet);

        fprintf(stderr, "print STATE before\n");
        print_state();

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
                break;
            case OPCODE_ACK:
                break;
            default:
                break;
        }

        if (game.cur_player == -1 && game.state == STATE_PLAY) {//update for play state
            fprintf(stderr, "play round start, player:%d\n", game.cur_player);
            next_player(game.cur_player);
            set_timer();
        } else if (game.state == STATE_FINISH) {//finished state and first time in it -- use game.finish_resend
            fprintf(stderr, "in final state\n");
            //make dealers moves
            dealer_play();
            game.state = STATE_IDLE;//REMOVE
            //update money
            round_end();
            //send updated board
            send_request();//keep one
            send_request();//after a while send an updated one with the proper client its waiting for the next round

            kick_bankrupt();//MOVE
            remove_kicked();//MOVE
            reset_game();//MOVE
            fprintf(stderr, "round reset\n");
        }
        if (game.state != STATE_IDLE) {//move out once resend timer is working
            fprintf(stderr, "send_request\n");
            send_request();// do i want to move this to be timed
        }
        //if i need to start the round from idle
        if (game.cur_player == -1 && game.state == STATE_IDLE && num_players() != 0) {
            //set players to be active
            set_players_active();
            fprintf(stderr, "start round\n");
            //set state
            game.state = STATE_BET;
            //set player
            next_player(-1);//deal with being kicked or not active
            set_timer();
            //send request to player
        }
        print_state();

        //check timers
        check_timers();
    }

    close(sfd);
}
