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

#include <sys/socket.h>
#include <sys/time.h>

#define MAX_PLAYERS     7
#define MAX_NUM_CARDS   21

// default defines
#define DEFAULT_BET     1
#define DEFAULT_START   100
#define DEFAULT_TIME    15
#define DEFAULT_DECKS   2

#define PLAYER_A_ACTIVE   1
#define PLAYER_A_INACTIVE 0
#define PLAYER_A_KICKED   -1

#define PLAYER_NAME_LEN 12

#define PORT_LEN        6

#define FINISH_SEND_THRESHOLD   3

/* user struct for storing persistand user data */
typedef struct user_s {
    char *name;
    uint32_t money;
} user_s;

/* userlist struct that holds persistand user data */
typedef struct userlist_s {
    user_s **users;
    int cur_users;
    int max_users;
} userlist_s;

/* game rules struct that holds resend timer delays and other infomation
 *  regarding the server and player joining */
typedef struct game_rules {
    int decks;
    int time;
    uint32_t start;
    uint32_t min_bet;
    char port[PORT_LEN];
} game_rules;

/* player struct holds all infomation for connected players */
typedef struct player_s {
    char *nick;
    uint32_t money;
    uint32_t bet;
    uint8_t cards[MAX_NUM_CARDS];
    int num_cards;
    int active; //uses PLAYER_A_ defines
    struct sockaddr_storage sock;
} player_s;

/* the deck struct holds the cards */
typedef struct deck_s {
    uint8_t *cards;
    int cur_card;
    int num_cards;
} deck_s;

/* state enum */
typedef enum game_state {
    STATE_IDLE,
    STATE_BET,
    STATE_PLAY,
    STATE_FINISH
} game_state;

/* main game state struct, holds all players, the dealer and state infomation */
typedef struct game_s {
    deck_s deck;
    uint8_t d_cards[MAX_NUM_CARDS];
    int d_shown_cards;
    int d_num_cards;
    player_s *players[MAX_PLAYERS];
    int num_players;
    int max_players;
    uint16_t seq_num;
    game_state state;
    int cur_player;
    struct timeval kick_timer;
    int finish_resend;
    struct timeval resend_timer;
} game_s;

// extern globals
extern int verbosity;

extern userlist_s userlist;
extern game_rules rules;
extern game_s game;

/* deletes a player from the game struct and puts them in the userlist */
int delete_player(int i);

/* initializes the game struct */
void init_game();

/* frees the game struct */
void free_game();

/********************
 * player functions *
 ********************/

/* adds the given player to the game or returns an error */
int add_player(char *player_name, struct sockaddr_storage store);

int64_t get_player_money(int i);

int get_player(char *nick);

int get_player_sock(struct sockaddr_storage store);

int num_players();

void kick_player(int p);

int valid_nick(char *nick);

void next_player(int cur);

void set_players_active();

// deck functions
int init_deck();

void free_deck();

void deal_cards();

int hit(int p);

int dealer_play();

// userlist funcitons
int init_userlist();

void free_userlist();

uint64_t get_user_money(char *nick);

int get_user(char *nick);

int add_player_to_list(char *nick, uint32_t money);

//round end

void round_end();

void kick_bankrupt();

void remove_kicked();

void reset_game();

//timer

void set_timer();

int check_kick();

void set_resend_timer();

int check_resend_timer();

#endif /* GAME_H */
