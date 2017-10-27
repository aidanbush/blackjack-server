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

static int init_server() {
    struct addrinfo *res, hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
        .ai_flags = AI_PASSIVE
    };
    int err, sfd;

    err = getaddrinfo(NULL, PORT, &hints, &res);
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

static int send_error(uint8_t error_opcode, struct sockaddr_storage dest, char *msgstr) {
    uint8_t packet[ERROR_LEN];
    //memset 0
    memset(&packet, 0, ERROR_LEN);
    packet[0] = OPCODE_ERROR;
    packet[1] = error_opcode;
    //copy str
    strncpy((char *) &packet[2], msgstr, ERROR_MSG_LEN);

    return 1;
}

/*int ack(packet) {
    //if acked msg exists
        //remove acked msg from buffer
        //return success
    //else send error msg
        //return error
}*/

/*int quit(packet) {
    //set player kicked
}*/

/*int hit(packet) {
    //check if in proper state -- may want to move into main loop
        //if not return failure
    //check if current player is active player
    //add card
    //check if bust
        //return bust
    //return not bust
}*/

/*int stand(packet) {
    //check if in proper state
        //if not return failure
    //check if current player is active player
        //if not return error
    //return stand
}*/

/*int msg(packet) {
    //check if in proper state
        //if not return failure
    //add to ring buffer
    //if failed return return error
    //return success
}*/

/*int bet(packet) {
    //check if in proper state
        //if not return failure
    //check if player already bet
    //set bet
    //return success
}*/

static int op_connect(uint8_t *packet, int len, struct sockaddr_storage recv_store) {
    //if no space
    if (game.num_players >= game.max_players) {
        //send error message
        return -1;
    }

    // packet length incorrect
    if (len != CONNECT_LEN) {
        //send error message
        //close connection???
        //return error
        return -1;
    }

    char *nick = get_connect_nick(packet);
    //copy nick into array on stack with null terminator;
    char new_nick[PLAYER_NAME_LEN + 1]; // TODO test---------------------
    if (strncpy(new_nick, nick, PLAYER_NAME_LEN) == NULL) {
        return -1;
    }
    new_nick[PLAYER_NAME_LEN] = '\0';
    // verify nickname
    if (valid_nick(new_nick) == -1) {
        //send error message
        return -1;
    }

    //add player
    if (add_player(nick, recv_store) == -1) {//need to also add connection information
        return -1;//if error return error dont respond to save memory???--------------
    }

    return 1;//success
}

void server() {
    uint8_t packet[MAX_PACKET_LEN];

    int sfd = init_server();
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

        nrdy = select(sfd, &readfds, NULL, NULL, &timeout);
        if (nrdy == -1) {
            printf("ERROR\n");
            continue;
        }
        //check error
        if (nrdy == 0) {
            printf("TIMEOUT\n");
            continue;
        }

        printf("got msg\n");
        recv_len = recvfrom(sfd, &packet, sizeof(packet), 0,
                            (struct sockaddr*) &recv_store, &recv_addr_len);

        opcode = get_opcode(packet);

        //switch on opcode
        switch (opcode) {
            //each opcode calls different function
            case OPCODE_CONNECT:
                op_connect(packet, recv_len, recv_store);
                break;
            case OPCODE_BET:
                break;
            case OPCODE_STAND:
                break;
            case OPCODE_HIT:
                break;
            case OPCODE_QUIT:
                break;
            case OPCODE_MESSAGE:
                break;
            case OPCODE_ACK:
                break;
            default:
                break;
        }
        //if time up kick
        //if time up forward messages (messages are only send here)
        //check if state needs to be updated
    }

    close(sfd);
}
