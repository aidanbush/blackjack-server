/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct. 19, 17
 * File: game.h
 * Description: the main game logic, also deals with the persistent user data,
 *  the connected players, and timers
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

// player active status defines
#define PLAYER_A_ACTIVE   1
#define PLAYER_A_INACTIVE 0
#define PLAYER_A_KICKED   -1

#define PLAYER_NAME_LEN 12

#define PORT_LEN        6

#define FINISH_SEND_THRESHOLD   3

/* user struct for storing persistent user data */
typedef struct user_s {
    char *name;
    uint32_t money;
} user_s;

/* userlist struct that holds persistent user data */
typedef struct userlist_s {
    user_s **users;
    int cur_users;
    int max_users;
} userlist_s;

/* game rules struct that holds resend timer delays and other information
 *  regarding the server and player joining */
typedef struct game_rules {
    int decks;
    int time;
    uint32_t start;
    uint32_t min_bet;
    char port[PORT_LEN];
} game_rules;

/* player struct holds all information for connected players */
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

/* main game state struct, holds all players, the dealer and state information */
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

/* deletes a player from the game struct and puts them in the userlist */
int delete_player(int i);

/* initializes the game struct */
void init_game();

/* frees the game struct */
void free_game();

/********************/
/* player functions */
/********************/

/* adds the given player to the game or returns an error */
int add_player(char *player_name, struct sockaddr_storage store);

/* returns the players money for the given player index, or -1 if they don't
 *  exist*/
int64_t get_player_money(int i);

/* gets the player index for the given nickname, otherwise returns -1 */
int get_player(char *nick);

/* sets the given players active status to kicked */
void kick_player(int p);

/* tests if a nickname is valid returning -1 if its not */
int valid_nick(char *nick);

/* returns a players index for the given socket, returns -1 if they don't exist */
int get_player_sock(struct sockaddr_storage store);

/* returns the number of players in the game, regardless of their active value */
int num_players();

/* returns the player index of the active player after the given index
 * returns -1 if they are the last in the list */
void next_player(int cur);

/* sets all players to active */
void set_players_active();

/******************/
/* deck functions */
/******************/

/* initializes the deck and returns number of cards added to the deck */
int init_deck();

/* frees the deck and reset its values */
void free_deck();

/* deals the first two cards to each player and the dealer, and sets their
 *  number of cards */
void deal_cards();

/* deals a card to the given player and returns -1 on bust otherwise -1*/
int hit(int p);

/* plays out the dealers turn having them hit on a soft 17 returning -1 if they
 *  bust otherwise 1 */
int dealer_play();

/**********************/
/* userlist functions */
/**********************/

/* initializes the userlist */
int init_userlist();

/* frees the userlist */
void free_userlist();

/* returns the users money from the userlist using a given nickname, otherwise
 *  returns -1 on error */
uint64_t get_user_money(char *nick);

/* retrieves the users position in userlist for the given nickname */
int get_user(char *nick);

/* adds the given player to the userlist and sets their money accordingly */
int add_player_to_list(char *nick, uint32_t money);

/***********************/
/* round end functions */
/***********************/

/* calculates the end of the round and distributes winnings/losses */
void round_end();

/* set all bankrupt players to kicked */
void kick_bankrupt();

/* removed kicked players */
void remove_kicked();

/* reset the game, including a players, and the dealer does not modify state */
void reset_game();

/*******************/
/* timer functions */
/*******************/

/* set the kick timer for the next kick */
void set_timer();

/* checks if the current player needs to be kicked for timeout, and kicks them
 *  if so also updates the state */
int check_kick();

/* sets the resend timer */
void set_resend_timer();

/* returns 1 if the resend timer is up */
int check_resend_timer();

#endif /* GAME_H */
