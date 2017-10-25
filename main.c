/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct. 19, 17
 * File: main.c
 * Description: main program file
 */

/* standard libraries */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>

/* system libraries */

/* project includes */
#include "game.h"

int v = 0;
int port = 4420;
game_rules rules;
game_s game;
userlist_s userlist;

/* prints the usage message and returns */
void print_usage(char *p_name) {
    printf("Usage : %s  [-options]\n"
        "Blackjack server as specified in RFC n\n\n"
        "Options:\n"
        "    -d number of decks default 2\n"
        "    -m amount of money default 100\n"
        "    -t the round time limit (10s < t < 45s, default is 15 seconds)\n"
        "    -p port default 4420\n"
        "    -b minimum bet (b >= 1, default 1)\n"
        "    -h displays usage message\n", basename(p_name));
}

int main(int argc, char **argv) {
    int c;
    rules.decks = DEFAULT_DECKS;
    rules.time = DEFAULT_TIME;
    rules.start = DEFAULT_START;
    rules.min_bet = DEFAULT_BET;

    while ((c = getopt(argc, argv, "vhd:m:t:p:b:")) != -1) {
        switch (c) {
            case 'v':
                v++;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
                break;
            case 'd':
                printf("d:%d\n", atoi(optarg));
                break;
            case 'm':
                printf("m:%d\n", atoi(optarg));
                break;
            case 't':
                printf("t:%d\n", atoi(optarg));
                break;
            case 'p':
                printf("p:%d\n", atoi(optarg));
                break;
            case 'b':
                printf("b:%d\n", atoi(optarg));
                break;
            case ':':
                printf("missing arg\n");
                break;
            case '?':
                printf("unkown option\n");
            default:
                printf("error\n");
                break;
        }
    }

    init_game();
    init_deck();
    init_userlist();

    // call main loop

    free_game();
    free_deck();
    free_userlist();

    return 0;
}
