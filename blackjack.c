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
#include "msg.h"
#include "file.h"

// globals for tracking game logic and persistent data
int verbosity = 0;
game_rules rules;
game_s game;
userlist_s userlist;

extern msg_list msg_queue;
extern msg_ack_list msg_ack_queue;

/* prints the usage message and returns */
void print_usage(char *p_name) {
    printf("Usage : %s  [-options] [persistent data file]\n"
        "Blackjack server as specified in RFC n + 21\n\n"
        "Options:\n"
        "    -v verbose [use more than once for higher levels]\n"
        "        1 error messages\n"
        "        2 limited message logging\n"
        "        3 state changes and limited game logic\n"
        "        4 all messages logging\n"
        "        5 extra state information\n"
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
    char *filename = NULL;

    // setup rules with defaults
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
                    snprintf(rules.port, PORT_LEN, "%ld", opt);
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

    // if the minimum bet is smaller than the starting money
    if (rules.min_bet > rules.start)
        inval = 1;

    // if any invalid options were given
    if (inval == 1) {
        print_usage(argv[0]);
        return 1;
    }

    if (optind >= 1) {
        filename = argv[optind];
    }

    // setup game
    init_game();
    init_deck();
    init_userlist();

    //setup message queue
    init_msg_queue();

    //read from file
    read_userlist_file(filename);

    // call main loop
    server();

    //write to file
    write_userlist_file(filename);

    // tear down game
    free_game();
    free_deck();
    free_userlist();

    free_msg_queue();

    return 0;
}
