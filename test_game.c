/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date:
 * File:
 * Description:
 */

/* standard libraries */
#include <stdio.h>
#include <stdlib.h>

/* system libraries */
#include <arpa/inet.h>

/* project includes */
#include "game.h"
#include "packet.h"

userlist_s userlist;
game_rules rules;
game_s game;

void add_7_players() {
    char name[12];
    for (int i = 0; i < 6; i++) {
        // create name string
        sprintf(name, "new_t %d", i);
        // add player
        add_player(name);
    }
    // add 7th
    add_player("new_t fail");
}

int main() {
    printf("creating test packet and setting bet\n");
    uint8_t *p;
    p = create_status_packet(2);
    if (p == NULL) {
        fprintf(stderr, "\tfailed to create packet\n");
    } else {
        *((uint32_t *)(p+1)) = htonl(1000);// assign bet
        if (get_bet(p) == 1000)
            printf("\tcorrect bet assigned\n");
        else
            fprintf(stderr, "\tERROR: incorrect bet\n");
        free(p);
    }
    // call init functions
    printf("init globals\n");
    rules.decks = DEFAULT_DECKS;
    rules.time = DEFAULT_TIME;
    rules.start = DEFAULT_START;
    rules.min_bet = DEFAULT_BET;

    init_game();
    init_deck();
    init_userlist();

    // add player
    printf("creating player\n");
    if (add_player("test1") < 1)
        fprintf(stderr, "\tERROR: in creating player\n");

    // test if player was added
    int test1_p = get_player("test1");
    if (test1_p > -1)
        printf("\tsuccessfully created player\n");
    else
        fprintf(stderr, "\tERROR: in creating player\n");

    printf("testing if the players money is correct\n");
    int64_t test_money = get_player_money(test1_p);
    if (test_money == DEFAULT_START)
        printf("\tcorrect money\n");
    else
        fprintf(stderr, "\tERROR: in money\n");

    // add 7 more players
    add_7_players();

    // play card
    // modify money

    // delete player
    printf("deleting player\n");
    if (delete_player(test1_p) > -1)
        printf("\tsuccessfully deleted player\n");
    else
        fprintf(stderr, "\tERROR: in adding player to userlist\n");

    printf("testing if user was added correctly to userlist\n");
    int user_i = get_user("test1");
    if (user_i > -1)
        printf("\tplayer added\n");
    else
        fprintf(stderr, "\tERROR: player was not succesfully added\n");

    printf("testing if player has correct balance in userlist\n");
    uint32_t money = get_user_money(user_i);
    if (money == test_money)
        printf("\tcorrect balance\n");
    else
        fprintf(stderr, "\tERROR: balance error\n");

    // free
    printf("free functions\n");
    free_game();
    free_deck();
    free_userlist();

    // try and recreate
    return 0;
}
