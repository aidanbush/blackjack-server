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

//should be static
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
    player->active = 0;
    player->sock = store;
    player->num_cards = 0;

    return player;
}

static void free_player(player_s *p) {
    free(p->nick);
    free(p);
}

// moves player into userlist and then frees them
// if returns response code from add_player_to_list
int delete_player(int i) {
    player_s *p = game.players[i];
    if (p == NULL) {
        fprintf(stderr, "p == NULL\n");
        return -1;
    }
    int add = add_player_to_list(p->nick, p->money);
    free_player(p);
    game.players[i] = NULL;
    return add;
}

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
    game.waiting = 0;
}

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

//returns index if added otherwise -2 if nick already taken, -1 if there was an error
int add_player(char *player_name, struct sockaddr_storage store) {
    //check if player with same nick already there
    if (get_player(player_name) != -1) {
        return -2;
    }

    int64_t test_money;
    uint32_t money;//test -----------------------------------------
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

// int64_t could introduce bugs could switch with
int64_t get_player_money(int i) {
    if (game.players[i] == NULL)
        return -1;
    return game.players[i]->money;
}

int get_player(char *nick) {
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL)
            if (strcmp(nick, game.players[i]->nick) == 0)
                return i;
    return -1;
}

void kick_player(int p) {
    if (game.players[p] == NULL)
        return;
    game.players[p]->active = -1;
}

int valid_nick(char *nick) {
    //loop through everything
    for (int i = 0; i < PLAYER_NAME_LEN && nick[i] != '\0'; i++) {
        if (!isalnum(nick[i]))//if not alphanumeric
            return -1;
    }
    return 1;
}

int sock_cmp(struct sockaddr_storage fst, struct sockaddr_storage snd) {
    return memcmp(&fst, &snd, sizeof(struct sockaddr_storage));
}

int get_player_sock(struct sockaddr_storage store) {
    //for all spots
    for (int  i = 0; i < game.max_players; i++)
        //if player exists
        if (game.players[i] != NULL)
            //if sock matches
            if (sock_cmp(game.players[i]->sock, store) == 0)
                return i;
    return -1;
}

int num_players() {
    int c = 0;
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL)
            c++;
    return c;
}

//return next player id
int next_player(int cur) {// refactor to deal with game.cur_player
    if (cur >= game.max_players)
        return -1;

    if (cur < -1) cur = -1;

    for (int i = cur+1; i < game.max_players; i++) {
        if (game.players[i] != NULL)
            if (game.players[i]->active == 1)//if active
                return i;
    }
    return -1;
}

void set_players_active() {
    //for all players
    for (int i = 0; i < game.max_players; i++)
        //if not null
        if (game.players[i] != NULL)
            game.players[i]->active = 1;
}

// DECK FUNCTIONS

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

void free_deck() {
    //free deck
    free(game.deck.cards);
    //set null
    game.deck.cards = NULL;
}

//grabs one card from the deck and returns it
static int get_card() {
    return game.deck.cards[game.deck.cur_card++];
}

//deals two cards to each player and the dealer
void deal_cards() {
    //dealer
    game.d_cards[0] = get_card();
    game.d_cards[1] = get_card();
    game.d_shown_cards = 1;
    game.d_num_cards = 2;
    //players
    for (int i = 0; i < game.max_players; i++) {
        //if player exists
        if (game.players[i] != NULL) {
            game.players[i]->cards[0] = get_card();//deal cards
            game.players[i]->cards[1] = get_card();
            game.players[i]->num_cards = 2;
        }
    }
}

//retunr 1 if the given player has a blackjack
static int blackjack(int p) {
    if (game.players[p] == NULL) {
        return -1;
    }
    //if player was not dealt
    if (game.players[p]->cards[0] == 0 || game.players[p] == 0)
        return -1;

    //if the player has blackjack
    if ((game.players[p]->cards[0] + game.players[p]->cards[1]) == 21)
        return 1;
    return -1;
}

static int d_blackjack() {
    if ((game.d_cards[0] + game.d_cards[1]) == 21)
        return 1;
    return -1;
}

static uint8_t card_value(uint8_t card) {
    uint8_t value = ((card - 1) % 13) + 1;
    if (value > 10)//if face card
        value = 10;
    if (value == 1)//if ace
        value = 11;
    return value;
}

//-1 if player does not exist
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

//return -1 if bust, 1 otherwise
//if the player is already busted returns -1
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

//soft 17
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

//soft 17
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

// deal with being called multiple times
int init_userlist() {
    userlist.users = calloc(START_USER_LIST_LEN, sizeof(user_s));
    if (userlist.users == NULL)
        return -1;

    userlist.cur_users = 0;
    userlist.max_users = START_USER_LIST_LEN;
    return 1;
}

static void free_user(user_s *user) {
    free(user->name);
    free(user);
}

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

// TODO replace with real hash function
static int hash(char *str) {
    int h = 0;
    for (int i = 0; i < strlen(str); i++) {
        h += str[i];
    }
    return h;
}

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
            hash_user(old_users[i]); // TODO return value here
    // free old array
    free(old_users);
    return 1;
}

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

int get_user(char *name) {
    for (int i = 0; i < userlist.max_users; i++)
       if (userlist.users[i] != NULL)
            if (strncmp(name, userlist.users[i]->name, PLAYER_NAME_LEN) == 0)
                return i;
    return -1; // not found
}

int add_player_to_list(char *nick, uint32_t money) {
    // if no space and must be reinited
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
    //add nul terminator to be safe
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

static void player_tie(int p) {
    game.players[p]->bet = 0;//reset bet
}

//lose bet
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

//calculates who won and how much they win
void round_end() {
    //get dealers hand
    int d_value = d_hand_value();
    if (d_value > 21)// if bust
        d_value = -1;

    int d_black = d_blackjack();
    int p_value;
    //for every player
    for (int i = 0; i < game.max_players; i++) {
        //if not null
        if (game.players[i] != NULL && game.players[i]->active != 0) {
            p_value = player_hand_value(i);
            if (p_value > 21)// if bust
                p_value = -1;
            fprintf(stderr, "results for player :%d\n", i);
            if (blackjack(i) == 1) {// if blackjack
                if(d_black == 1) {// if both no one wins
                    fprintf(stderr, "both blackjack\n");
                    player_tie(i);//return money
                } else {
                    fprintf(stderr, "player blackjack\n");
                    player_blackjack(i);//win 1.5x money
                }
            } else if (p_value > d_value) {//else if above
                fprintf(stderr, "player win\n");
                player_win(i);//win money
            } else if (p_value == d_value) {//else if tie
                fprintf(stderr, "tie\n");
                player_tie(i);//return money
            } else {//else lose
                fprintf(stderr, "player losses\n");
                player_lost(i);//lose money
            }
        }
    }
    //reset deck if needed ---TODO may want to move into separate function
    if (game.deck.cur_card > (game.deck.num_cards / 2)) {
        shuffle_cards();
        game.deck.cur_card = 0;
    }
}

void kick_bankrupt() {
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL && game.players[i]->money < rules.min_bet) {
            fprintf(stderr, "kicking %d due to insufficient funds\n", i);
            game.players[i]->active = -1;
        }
}

void remove_kicked() {
    //for all players
    for (int i = 0; i < game.max_players; i++)
        //if player != NULL
        if (game.players[i] != NULL)
            //if kicked, delete
            if (game.players[i]->active == -1)
                //delete player
                delete_player(i);
}

static void reset_bets() {
    //for all players
    for (int i = 0; i < game.max_players; i++)
        //if player != NULL
        if (game.players[i] != NULL)
            //set bet to 0
            game.players[i]->bet = 0;
}

static void reset_player(int p) {
    if (game.players[p] == NULL)
        return;
    //zero out deck
    for (int i = 0; i < MAX_NUM_CARDS; i++)//may only have to set num cards to 0
        game.players[p]->cards[i] = 0;
    //num_cards == 0
    game.players[p]->num_cards = 0;
}

static void reset_dealer() {
    //for cards
    for (int i = 0; i < MAX_NUM_CARDS; i++)
        game.d_cards[i] = 0;
    //card count
    game.d_num_cards = 0;
    //shown cards
    game.d_shown_cards = 0;
}

//may be redundant but still a good check
static void set_num_players() {
    int c = 0;
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL)
            c++;
    game.num_players = c;
}

void reset_game() {
    //reset players cards and num cards
    for (int i = 0; i < game.max_players; i++)
        if (game.players[i] != NULL)
            reset_player(i);
    //reset bets
    reset_bets();
    //reset dealers cards and num cards
    reset_dealer();
    //curent palyer
    game.cur_player = -1;
    set_num_players();
}
