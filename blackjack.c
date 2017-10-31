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
#include <stdint.h>

/* system libraries */

/* project includes */
#include "game.h"
#include "server.h"

int verbosity = 0;
game_rules rules;
game_s game;
userlist_s userlist;

/* prints the usage message and returns */
void print_usage(char *p_name) {
    printf("Usage : %s  [-options]\n"
        "Blackjack server as specified in RFC n + 21\n\n"
        "Options:\n"
        "    -v verbosity\n"
        "    -d number of decks default 2\n"
        "    -m amount of money default 100\n"
        "    -t the round time limit (10s < t < 45s, default is 15 seconds)\n"
        "    -p port default 4420\n"
        "    -b minimum bet (b >= 1, default 1)\n"
        "    -h displays usage message\n", basename(p_name));
}

int main(int argc, char **argv) {
    int c, inval = 0;
    int64_t opt;

    rules.decks = DEFAULT_DECKS;
    rules.time = DEFAULT_TIME;
    rules.start = DEFAULT_START;
    rules.min_bet = DEFAULT_BET;
    snprintf(rules.port, PORT_LEN - 1, "%d", 4420);

    while ((c = getopt(argc, argv, "vhd:m:t:p:b:")) != -1) {
        switch (c) {
            case 'v':
                verbosity++;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
                break;
            case 'd':
                opt = atoi(optarg);
                if (opt > 0 && opt <= 10)
                    rules.decks = opt;
                else
                    inval = 1;
                break;
            case 'm':
                opt = atol(optarg);
                if (opt > 0 && opt <= UINT32_MAX)
                    rules.start = opt;
                else
                    inval = 1;
                break;
            case 't':
                opt = atoi(optarg);
                if (opt >= 10 && opt <= 45)
                    rules.time = opt;
                else
                    inval = 1;
                break;
            case 'p':
                opt = atoi(optarg);
                if (opt != 0)
                    snprintf(rules.port, PORT_LEN - 1, "%ld", opt);
                else
                    inval = 1;
                break;
            case 'b':
                opt = atol(optarg);
                if (opt >= 0 && opt <= UINT32_MAX)
                    rules.min_bet = opt;
                else
                    inval = 1;
                break;
            case ':':
            case '?':
            default:
                inval = 1;
                break;
        }
    }

    if (rules.min_bet > rules.start)
        inval = 1;

    if (inval == 1) {
        //free anything needed
        print_usage(argv[0]);
        return 1;
    }

    init_game();
    init_deck();
    init_userlist();

    // call main loop
    server();

    free_game();
    free_deck();
    free_userlist();

    return 0;
}
