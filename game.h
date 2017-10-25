/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct. 19, 17
 * File: game.h
 * Description: main game logic
 */

#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_PLAYERS 7

#define MAX_NUM_CARDS 21
#define DEFAULT_BET 1
#define DEFAULT_START 100
#define DEFAULT_TIME 10
#define DEFAULT_DECKS 2

#define PLAYER_NAME_LEN 12

typedef struct user_s {
    char *name;
    uint32_t money;
} user_s;

typedef struct userlist_s {
    user_s **users;
    int cur_users;
    int max_users;
} userlist_s;

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
    //add players socket socket
} player_s;

typedef struct deck_s {
    uint8_t *cards;
    int cur_card;
    int num_cards;
} deck_s;

typedef struct game_s {
    deck_s deck;
    uint8_t d_cards[MAX_NUM_CARDS];
    player_s *players[MAX_PLAYERS];
    int num_players;
    int max_players;
} game_s;

extern userlist_s userlist;
extern game_rules rules;
extern game_s game;

player_s *init_player(char *nick, uint32_t start_money);

int delete_player(int i);

void init_game();

void free_game();

// player functions
int add_player(char *player_name);

int64_t get_player_money(int i);

int get_player(char *nick);

void kick_player(player_s *p);

// deck functions
int init_deck();

void free_deck();

// userlist funcitons
int init_userlist();

void free_userlist();

uint64_t get_user_money(int i);

int get_user(char *name);

int add_player_to_list(char *name, uint32_t money);

#endif /* GAME_H */
