/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct. 19, 17
 * File: game.h
 * Description: main game logic
 */

#ifndef _H
#define _H

#include <stdint.h>

typedef struct game_rules {
    int decks;
    int time;
    uint32_t start;
    uint32_t min_bet;
} game_rules;

#endif /* _H */
