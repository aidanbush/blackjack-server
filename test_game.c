/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date:
 * File:
 * Description:
 */

/* standard libraries */
#include <stdio.h>

/* system libraries */

/* project includes */
#include "game.h"

userlist_s userlist;
game_rules rules;
game_s game;

int main() {
    // call init functions
    rules.decks = DEFAULT_DECKS;
    rules.time = DEFAULT_TIME;
    rules.start = DEFAULT_START;
    rules.min_bet = DEFAULT_BET;
    init_game();
    init_deck();
    // create player
    // delete and move to userlist
    // cause userlist to be resized
    // free everything
    free_game();
    free_userlist();

    // try and recreate
    return 0;
}
