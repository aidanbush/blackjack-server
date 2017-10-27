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

/* system libraries */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

/* project includes */
#include "packet.h"

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

/*int connect(packet) {
    //if no space
        //send error message
        //close connection???
        //return error

    //add player
    //if error return error

    //set inactive

    //return success
}
*/

void server() {
    int sfd = init_server();
    if (sfd == -1) {
        fprintf(stderr, "unable to create server\n");
        return;
    }

    fd_set mfds, readfds;
    FD_ZERO(&mfds);
    FD_SET(sfd, &mfds);

    int nrdy;
    struct timeval timeout;

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
        recvmsg(sfd, NULL, 0);

        //switch on opcode
            //each opcode calls different function
        //if time up kick
        //if time up forward messages (messages are only send here)
        //check if state needs to be updated
    }

    close(sfd);
}
