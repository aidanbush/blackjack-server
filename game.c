/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct. 19, 17
 * File: game.c
 * Description: main game logic
 */

/* standard libraries */
#include <string.h>
#include <stdbool.h>

/* system libraries */

/* project includes */
#include "game.h"

extern game_rules rules;

player_s *init_player(char *nick, uint32_t start_money) {
	player_s *player = malloc(sizeof(player_s));
	if (player == NULL)
		return NULL;

	player->nick = malloc(sizeof(char) * (PLAYER_NAME_LEN + 1));
	if (player->nick == NULL) {
		free(player);
		return NULL;
	}

	// copy over if failed
	if (strncpy(player->nick, nick, PLAYER_NAME_LEN) == NULL) {
		free(player->nick);
		free(player);
		return NULL;
	}

	for (int i = 0; i < MAX_NUM_CARDS; i++)
		player->cards[i] = 0;
	player->money = start_money;
	player->bet = 0;
	player->kicked = false;

	return player;
}

void free_player(player_s *p) {
	free(player->nick);
	free(player);
}

game_s *init_game() {
	game_s game = malloc(sizeof(game_s));
	if (game == NULL)
		return NULL;

	for (int i = 0; i < MAX_NUM_CARDS; i++)
		game->d_cards[i] = 0;

	for (int i = 0; 0< MAX_PLAYERS; i++) game->players[i] = NULL;

	game->num_players = 0;
	game->max_players = MAX_PLAYERS;

	return game;
}

int add_player(game_s *game, char *player_name) {
	// check if nick is not out of money or their money is below min bet
	uint32_t money = rules.start;
	if (money < rules.min_bet)
		return -1;

	player_s *player = init_player(player_name, money);

	if (player == NULL)
		return NULL;

	for (int i = 0; i < game->max_players; i++) {
		if (game->players[i] != NULL) {
			game->players[i] = player;
			return 1;
		}
	}

	// no spot available
	free_player(player);
	return -1;
}

void kick_player(player_s *p) {
	p->kicked = true;
}
