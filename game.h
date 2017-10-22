/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct. 19, 17
 * File: game.h
 * Description: main game logic
 */

#ifndef _H
#define _H

#define MAX_PLAYERS 7

#define MAX_NUM_CARDS 21
#define DEFAULT_BET 1
#define DEFAULT_START 100
#define DEFAULT_TIME 10
#define DEFAULT_DECKS 2

#define PLAYER_NAME_LEN 12

#include <stdint.h>
#include <stdbool.h>

typedef struct game_rules {
    int decks;
    int time;
    uint32_t start;
    uint32_t min_bet;
} game_rules;

typedef struct player_s {
	char *nick;
	uint32_t money;
	uint32_t bet;
	uint8_t cards[MAX_NUM_CARDS];
	bool kicked;
} player_s;

typedef struct game_s {
	uint8_t d_cards[MAX_NUM_CARDS];
	player_s *players[MAX_PLAYERS];
	int num_players;
	int max_players;
} game_s;

#endif /* _H */
