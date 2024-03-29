/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct. 19, 17
 * File: game.c
 * Description: main game logic
 */

/* standard libraries */
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

// remove used for debugging
#include <stdio.h>

/* system libraries */
#include <sys/socket.h>

/* project includes */
#include "game.h"

#define START_USER_LIST_LEN 8

#define RESEND_DELAY    1// in seconds

// extern globals
extern int verbosity;

extern userlist_s userlist;
extern game_rules rules;
extern game_s game;

/* initializes a new player using the provided nickname, staring money, and socket */
static player_s *init_player(char *nick, uint32_t start_money, struct sockaddr_storage store) {
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
    player->active = PLAYER_A_INACTIVE;
    player->sock = store;
    player->num_cards = 0;

    return player;
}

/* frees a given player struct */
static void free_player(player_s *p) {
    free(p->nick);
    free(p);
}

/* moves player into userlist and then frees them if it failed to add to the
 *  userlist it forwards the response code from add_player_to_list*/
int delete_player(int i) {
    player_s *p = game.players[i];
    if (p == NULL) {
        return -1;
    }
    int add = add_player_to_list(p->nick, p->money);
    free_player(p);
    game.players[i] = NULL;
    game.num_players--;
    return add;
}

/* initializes the game struct */
void init_game() {
    for (int i = 0; i < (MAX_NUM_CARDS); i++)
        game.d_cards[i] = 0;

    game.d_shown_cards = 0;

    for (int i = 0; i < (MAX_PLAYERS); i++)
        game.players[i] = NULL;

    game.num_players = 0;
    game.max_players = (MAX_PLAYERS);

    game.deck.cards = NULL;
    game.deck.cur_card = 0;
    game.deck.num_cards = 0;
    game.seq_num = 0;
    game.state = STATE_IDLE;
    game.cur_player = -1;

    game.finish_resend = 0;
    set_timer();
    //set resend timer
    set_resend_timer();
}

/* frees the game struct */
void free_game() {
    // free all players
    for (int i = 0; i < game.max_players; i++) {
        if (game.players[i] != NULL) {
            free_player(game.players[i]);
            game.players[i] = NULL;
            game.num_players--;
        }
    }
    // if something went wrong
    if (game.num_players != 0)
        game.num_players = 0;
}

// PLAYER_FUNCTIONS

/* add a new player to the game, using the given information
 *  returns index if added otherwise -2 if nick already taken, -1 if there was
 *  some other error*/
int add_player(char *player_name, struct sockaddr_storage store) {
    //check if player with same nick already there
    if (get_player(player_name) != -1) {
        return -2;
    }

    int64_t test_money;
    uint32_t money;
    if ((test_money = get_user_money(player_name)) == -1)
        money = rules.start;
    else
        money = test_money;

    if (money < rules.min_bet)
        return -1;

    player_s *player = init_player(player_name, money, store);

    if (player == NULL)
        return -1;

    for (int i = 0; i < game.max_players; i++) {
        if (game.players[i] == NULL) {
            game.players[i] = player;
            return i;
        }
    }

    // no spot available
    free_player(player);
    return -1;
}

/* returns the players money for the given player index, or -1 if they don't
 *  exist*/
int64_t get_player_money(int i) {
    if (game.players[i] == NULL)
        return -1;
    return game.players[i]->money;
}

/* gets the player index for the given nickname, otherwise returns -1 */
int get_player(char *nick) {
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL)
            if (strcmp(nick, game.players[i]->nick) == 0)
                return i;
    return -1;
}

/* sets the given players active status to kicked */
void kick_player(int p) {
    if (game.players[p] == NULL)
        return;
    game.players[p]->active = PLAYER_A_KICKED;
}

/* tests if a nickname is valid returning -1 if its not */
int valid_nick(char *nick) {
    //loop through everything
    for (int i = 0; i < PLAYER_NAME_LEN && nick[i] != '\0'; i++) {
        if (!isalnum(nick[i]))//if not alphanumeric
            return -1;
    }
    return 1;
}

/* compares two sockaddr_storage structs and returns 0 if they are equal */
static int sock_cmp(struct sockaddr_storage fst, struct sockaddr_storage snd) {
    return memcmp(&fst, &snd, sizeof(struct sockaddr_storage));
}

/* returns a players index for the given socket, returns -1 if they don't exist */
int get_player_sock(struct sockaddr_storage store) {
    //for all players
    for (int  i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL)
            if (sock_cmp(game.players[i]->sock, store) == 0)
                return i;
    return -1;
}

/* returns the number of players in the game, regardless of their active value */
int num_players() {
    int c = 0;
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL)
            c++;
    return c;
}

/* returns the player index of the active player after the given index
 * returns -1 if they are the last in the list */
void next_player(int cur) {
    if (cur >= game.max_players){
        game.cur_player =  -1;
        return;
    }

    if (cur < -1) cur = -1;

    for (int i = cur+1; i < game.max_players; i++)
        if (game.players[i] != NULL)
            if (game.players[i]->active == PLAYER_A_ACTIVE){//if active
                game.cur_player = i;
                return;
            }
    game.cur_player = -1;
}

/* sets all players to active */
void set_players_active() {
    //for all players
    for (int i = 0; i < game.max_players; i++)
        //if not null
        if (game.players[i] != NULL)
            game.players[i]->active = PLAYER_A_ACTIVE;
}

// DECK FUNCTIONS

/* shuffles the deck and resets the current card index */
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

/* initializes the deck and returns number of cards added to the deck */
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
    game.deck.num_cards = 52 * rules.decks;
    return game.deck.num_cards;
}

/* frees the deck and reset its values */
void free_deck() {
    free(game.deck.cards);
    game.deck.cur_card = 0;
    game.deck.num_cards = 0;
    game.deck.cards = NULL;
}

/* grabs one card from the deck and returns it */
static int get_card() {
    return game.deck.cards[game.deck.cur_card++];
}

/* deals the first two cards to each player and the dealer, and sets their
 *  number of cards */
void deal_cards() {
    //dealer
    game.d_cards[0] = get_card();
    game.d_cards[1] = get_card();
    game.d_shown_cards = 1;
    game.d_num_cards = 2;
    //players
    for (int i = 0; i < game.max_players; i++) {
        //if player exists
        if (game.players[i] != NULL && game.players[i]->active == PLAYER_A_ACTIVE) {//may cause issues if they have bet
            game.players[i]->cards[0] = get_card();//deal cards
            game.players[i]->cards[1] = get_card();
            game.players[i]->num_cards = 2;
        }
    }
}

/* returns the cards value for a given card */
static uint8_t card_value(uint8_t card) {
    uint8_t value = ((card - 1) % 13) + 1;
    if (value > 10)//if face card
        value = 10;
    if (value == 1)//if ace
        value = 11;
    return value;
}

/* returns 1 if the given player has a blackjack otherwise -1 */
static int blackjack(int p) {
    if (game.players[p] == NULL) {
        return -1;
    }
    //if player was not dealt
    if (game.players[p]->cards[0] == 0 || game.players[p]->cards[1] == 0)
        return -1;

    //if the player has blackjack
    if (card_value(game.players[p]->cards[0]) + card_value(game.players[p]->cards[1]) == 21)
        return 1;
    return -1;
}

/* returns 1 if the dealer has a blackjack, otherwise -1*/
static int d_blackjack() {
    if (card_value(game.d_cards[0]) + card_value(game.d_cards[1]) == 21)
        return 1;
    return -1;
}

/* returns the given players hand value, or -1 on error */
static int player_hand_value(int p) {
    if (game.players[p] == NULL) {
        return -1;
    }
    int h_ace = 0, value = 0;
    uint8_t card;
    for (int i = 0; i < MAX_NUM_CARDS || game.players[p]->cards[i] == 0; i++) {
        card = card_value(game.players[p]->cards[i]);
        if (card == 11)//if ace
            h_ace += 1;
        value += card;
        //check if too high
        if (value > 21) {
            if (h_ace > 0){
                h_ace--;
                value -= 10;
            }
        }
    }
    return value;
}

/* deals a card to the given player and returns -1 on bust otherwise -1*/
int hit(int p) {
    //if player exists
    if (game.players[p] == NULL) {
        return -1;
    }
    //check players hand value
    if (player_hand_value(p) > 21) {
        return -1;
    }

    //get card and give to player
    uint8_t card = get_card();
    //add card to hand
    game.players[p]->cards[game.players[p]->num_cards++] = card;
    //calculate value
    if (player_hand_value(p) > 21)
        return -1;
    return 1;
}

/* returns the dealers hand value for playing on a soft 17 */
static int d_hand_value() {
    int h_ace = 0, value = 0;
    uint8_t card;
    for (int i = 0; i < game.d_num_cards && i < MAX_NUM_CARDS; i++) {
        card = card_value(game.d_cards[i]);
        if (card == 11)
            h_ace++;
        value += card;

        if (value > 21 || value == 17) {//too high or soft 17
            if (h_ace > 0) {
                h_ace--;
                value -= 10;
            }
        }
    }
    return value;
}

/* plays out the dealers turn having them hit on a soft 17 returning -1 if they
 *  bust otherwise 1 */
int dealer_play() {
    int value = 0;
    //get current value
    value = d_hand_value();

    //while low value < 17
    while (value < 17) {
        // get new card and add to deck
        game.d_cards[game.d_num_cards++] = get_card();
        //update value
        value = d_hand_value();
    }
    //set shown cards
    game.d_shown_cards = MAX_NUM_CARDS;
    //return -1 for bust, or card value
    if (value > 21)
        return -1;
    return 1;
}

// USERLIST FUNCTIONS

/* initializes the userlist */
int init_userlist() {
    userlist.users = calloc(START_USER_LIST_LEN, sizeof(user_s));
    if (userlist.users == NULL)
        return -1;

    userlist.cur_users = 0;
    userlist.max_users = START_USER_LIST_LEN;
    return 1;
}

/* frees a given user */
static void free_user(user_s *user) {
    free(user->name);
    free(user);
}

/* frees the userlist */
void free_userlist() {
    //free all users
    for (int i = 0; i < userlist.max_users; i++)
        if (userlist.users[i] != NULL)
            free_user(userlist.users[i]);
    //free array
    free(userlist.users);

    userlist.cur_users = 0;
    userlist.max_users = 0;
    userlist.users = NULL;
}

/* simple hash function for the userlist */
static int hash(char *str) {
    int h = 0;
    for (int i = 0; i < strlen(str); i++) {
        h += str[i];
    }
    return h;
}

/* hash and add a user to the userlist returning the position they were hashed
 *  to*/
static int hash_user(user_s *user) {
    // if no room
    if (userlist.cur_users >= userlist.max_users)
        return -1;

    int h = hash(user->name) % userlist.max_users;

    // deal with error when cur_users is incorrect
    while (userlist.users[h] != NULL)
        h = (h + 1) % userlist.max_users;
    userlist.users[h] = user;
    return h;
}

/* resizes the userlist hashtable */
static int resize_userlist() {
    int old_size = userlist.max_users;
    int new_size = old_size * 2;
    // create new array
    user_s **new_users = calloc(new_size, sizeof(user_s *));
    if (new_users == NULL)
        return -1;

    // create old to grab from
    user_s **old_users = userlist.users;

    // set pointer to new array
    userlist.users = new_users;
    userlist.max_users = new_size;

    // rehash into array
    for (int i = 0; i < old_size; i++)
        if (old_users[i] != NULL)
            hash_user(old_users[i]);
    // free old array
    free(old_users);
    return 1;
}

/* returns the users money from the userlist using a given nickname, otherwise
 *  returns -1 on error */
uint64_t get_user_money(char *nick) {
    int i;
    if ((i = get_user(nick)) == -1)
        return -1;
    if (i < 0 || i > userlist.max_users)
        return -1;
    if (userlist.users == NULL || userlist.users[i] == NULL)
        return -1;
    return userlist.users[i]->money;
}

/* retrieves the users position in the hashtable for the given nickname */
int get_user(char *nick) {
    for (int i = 0; i < userlist.max_users; i++)
       if (userlist.users[i] != NULL)
            if (strncmp(nick, userlist.users[i]->name, PLAYER_NAME_LEN) == 0)
                return i;
    return -1; // not found
}

/* adds the given player to the userlist and sets their money accordingly */
int add_player_to_list(char *nick, uint32_t money) {
    // if no space and must be reinitialized
    if (userlist.max_users == 0)
        return -1;
    // check if already in if so update
    int i;
    if ((i = get_user(nick)) != -1) {
        userlist.users[i]->money = money;
        return i;
    }

    // check if >= 66% full
    if (userlist.cur_users >= userlist.max_users * 2 / 3)
        if (resize_userlist() == -1)
            return -1;

    // create user
    user_s *new_user = malloc(sizeof(user_s));
    if (new_user == NULL)
        return -1;

    // allocate space for name
    new_user->name = calloc(PLAYER_NAME_LEN + 1, sizeof(char));
    if (new_user->name == NULL) {
        free(new_user);
        return -1;
    }
    new_user->name = strncpy(new_user->name, nick, PLAYER_NAME_LEN);
    //add null terminator to be safe
    new_user->name[PLAYER_NAME_LEN] = '\0';
    new_user->money = money;

    // add
    if ((i = hash_user(new_user)) == -1) {
        free(new_user->name);
        free(new_user);
        return -1;
    }
    return i;
}

// ROUND COMPLETION

/* add the given players winnings for a blackjack win */
static void player_blackjack(int p) {
    //if player does not exist
    if (game.players[p] == NULL)
        return;
    int64_t tmp_money = game.players[p]->bet * 3 / 2;//bet *1.5
    if (tmp_money + game.players[p]->money > UINT32_MAX)// if overflow
        game.players[p]->money = UINT32_MAX;
    else
        game.players[p]->money += tmp_money;//may need to cast
    game.players[p]->bet = 0;// reset bet
}

/* add the given players winnings for a regular win */
static void player_win(int p) {
    //if player does not exist
    if (game.players[p] == NULL)
        return;
    int64_t tmp_money = game.players[p]->bet;
    if (tmp_money + game.players[p]->money > UINT32_MAX)// if overflow
        game.players[p]->money = UINT32_MAX;
    else
        game.players[p]->money += tmp_money;//may need to cast
    game.players[p]->bet = 0;// reset bet
}

/* resets the players bet for a tie */
static void player_tie(int p) {
    game.players[p]->bet = 0;
}

/* takes away the players money for a loss */
static void player_lost(int p) {
    //if player does not exist
    if (game.players[p] == NULL)
        return;
    int64_t tmp_money = game.players[p]->bet;
    if (game.players[p]->money - game.players[p]->bet < 0)//check if they have enough money
        game.players[p]->money = 0;
    else
        game.players[p]->money -= tmp_money;
    game.players[p]->bet = 0;// reset bet
}

/* calculates the end of the round and distributes winnings/losses */
void round_end() {
    //get dealers hand
    int d_value = d_hand_value();
    if (d_value > 21)// if bust
        d_value = -1;

    int d_black = d_blackjack();
    int p_value;
    //for every player
    for (int i = 0; i < game.max_players; i++) {
        //if current player is active
        if (game.players[i] != NULL && game.players[i]->active != PLAYER_A_INACTIVE) {
            p_value = player_hand_value(i);
            if (p_value > 21)// if bust
                p_value = -1;

            if (verbosity >= 3)
                fprintf(stderr, "results for player :%d\n", i);

            if (blackjack(i) == 1) {// if blackjack
                if(d_black == 1) {// if both no one wins
                    if (verbosity >= 3)
                        fprintf(stderr, "both blackjack\n");
                    player_tie(i);//return money
                } else {
                    if (verbosity >= 3)
                        fprintf(stderr, "player blackjack\n");
                    player_blackjack(i);//win 1.5x money
                }
            } else if (p_value > d_value) {//else if above
                if (verbosity >= 3)
                    fprintf(stderr, "player won\n");
                player_win(i);//win money
            } else if (p_value == d_value) {//else if tie
                if (verbosity >= 3)
                    fprintf(stderr, "tie\n");
                player_tie(i);//return money
            } else {//else lose
                if (verbosity >= 3)
                    fprintf(stderr, "player lost\n");
                player_lost(i);//lose money
            }
        }
    }
    //reset deck if needed
    if (game.deck.cur_card > (game.deck.num_cards / 2)) {
        shuffle_cards();
        game.deck.cur_card = 0;
    }
}

/* set all bankrupt players to kicked */
void kick_bankrupt() {
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL && game.players[i]->money < rules.min_bet) {
            if (verbosity >= 3)
                fprintf(stderr, "kicking %d due to insufficient funds\n", i);
            game.players[i]->active = PLAYER_A_KICKED;
        }
}

/* removed kicked players */
void remove_kicked() {
    //for all players
    for (int i = 0; i < game.max_players; i++)
        //if player != NULL
        if (game.players[i] != NULL)
            //if kicked, delete
            if (game.players[i]->active == PLAYER_A_KICKED)
                //delete player
                delete_player(i);
}

/* reset all players bets */
static void reset_bets() {
    //for all players
    for (int i = 0; i < game.max_players; i++)
        //if player != NULL
        if (game.players[i] != NULL)
            //set bet to 0
            game.players[i]->bet = 0;
}

/* reset the given players cards */
static void reset_player(int p) {
    if (game.players[p] == NULL)
        return;
    //zero out deck
    for (int i = 0; i < MAX_NUM_CARDS; i++)//may only have to set num cards to 0
        game.players[p]->cards[i] = 0;
    //num_cards == 0
    game.players[p]->num_cards = 0;
}

/* reset the dealers cards */
static void reset_dealer() {
    //for cards
    for (int i = 0; i < MAX_NUM_CARDS; i++)
        game.d_cards[i] = 0;
    //card count
    game.d_num_cards = 0;
    //shown cards
    game.d_shown_cards = 0;
}

/* set the number of players, redundant, but will catch errors */
static void set_num_players() {
    int c = 0;
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL)
            c++;
    game.num_players = c;
}

/* reset the game, including a players, and the dealer does not modify state */
void reset_game() {
    //reset players cards and num cards
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL)
            reset_player(i);
    //reset bets
    reset_bets();
    //reset dealers cards and num cards
    reset_dealer();
    //current player
    game.cur_player = -1;
    set_num_players();
}

// TIMER

/* return different in milliseconds */
static int time_dif(struct timeval tv1, struct timeval tv2) {
    return (tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000;
}

/* set the kick timer for the next kick */
void set_timer() {
    gettimeofday(&game.kick_timer, NULL);
    //update seconds
    game.kick_timer.tv_sec += rules.time;
}

/* checks the kick timer returning 1 if the time is up */
static int check_kick_timer() {
    struct timeval tmp;
    gettimeofday(&tmp, NULL);
    if (time_dif(game.kick_timer, tmp) < 0)
        return 1;
    return 0;
}

/* checks if the current player needs to be kicked for timeout, and kicks them
 *  if so also updates the state */
int check_kick() {
    //check if timer is up and the current player is not -1
    if (!(check_kick_timer() && game.cur_player != -1))
        return 0;

    if (verbosity >= 3)
        fprintf(stderr, "kicking player: %d\n", game.cur_player);
    //if in bet state and have not made a bet or idle state
    if (game.state == STATE_IDLE ||
                (game.state == STATE_BET
                && game.players[game.cur_player] != NULL
                && game.players[game.cur_player]->bet == 0)) {
        delete_player(game.cur_player);//delete
        if (num_players() == 0) {
            if (verbosity >= 3)
                fprintf(stderr, "state now STATE_IDLE, last player deleted\n");
            game.state = STATE_IDLE;
        }
    } else {
        kick_player(game.cur_player);// kick player
    }

    //set timer
    set_timer();
    //set next player
    next_player(game.cur_player);
    //return that someone was kicked or deleted
    return 1;
}

/* sets the resend timer */
void set_resend_timer() {
    gettimeofday(&game.resend_timer, NULL);
    game.resend_timer.tv_sec += RESEND_DELAY;
}

/* returns 1 if the resend timer is up */
int check_resend_timer() {
    struct timeval tmp;
    gettimeofday(&tmp, NULL);
    if (time_dif(game.resend_timer, tmp) < 0)
        return 1;
    return 0;
}
