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
#include <time.h>
#include <stdlib.h>

/* system libraries */

/* project includes */
#include "game.h"

player_s *init_player(char *nick, uint32_t start_money) {
	player_s *player = malloc(sizeof(player_s));
	if (player == NULL)
		return NULL;

	player->nick = malloc(sizeof(char) * (PLAYER_NAME_LEN + 1));
	if (player->nick == NULL) {
		free(player);
		return NULL;
	}

	// copy over if fails free space and return null
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
	free(p->nick);
	free(p);
}

void init_game() {
	for (int i = 0; i < MAX_NUM_CARDS; i++)
		game.d_cards[i] = 0;

	for (int i = 0; 0< MAX_PLAYERS; i++) game.players[i] = NULL;

	game.num_players = 0;
	game.max_players = MAX_PLAYERS;
}

void free_game() {
	// free all players
	for (int i = 0; i < game.max_players; i++) {
		if (game.players[i] != NULL) {
			free(game.players[i]);
			game.players[i] = NULL;
			game.num_players--;
		}
	}
	// if something went wrong
	if (game.num_players != 0)
		game.num_players = 0;
}

static void shuffle_cards() {
	srand(time(NULL));// seed random
	int tmp, r;
	//swap all cards with another random card
	for (int i = 0; i < rules.decks * 52; i++) {
		tmp = game.deck.cards[i];
		r = rand() % (rules.decks * 52);
		game.deck.cards[i] = game.deck.cards[r];
		game.deck.cards[r] = tmp;
	}
}

// returns number of cards added to the deck
int init_deck() {
	game.deck.cards = malloc(sizeof(uint8_t) * 52 * rules.decks);
	if (game.deck.cards == NULL)
		return -1;

	// add cards
	for (int i = 0; i < rules.decks; i++) {
		for (int j = 1; j <= 52; j++) {
			game.deck.cards[i * 52 + j - 1] = j;
		}
	}

	shuffle_cards();
	return 52 * rules.decks;
}

int add_player(char *player_name) {
	// check if nick is not out of money or their money is below min bet
	uint32_t money = rules.start;
	if (money < rules.min_bet)
		return -1;

	player_s *player = init_player(player_name, money);

	if (player == NULL)
		return -1;

	for (int i = 0; i < game.max_players; i++) {
		if (game.players[i] != NULL) {
			game.players[i] = player;
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
