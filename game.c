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

#define START_USER_LIST_LEN 8

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

    for (int i = 1; 0< MAX_PLAYERS; i++)
        game.players[i] = NULL;

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

// deal with being called multiple times
int init_user_list() {
    userlist.users = calloc(START_USER_LIST_LEN, sizeof(user_s));
    if (userlist.users == NULL)
        return -1;

    userlist.cur_users = 0;
    userlist.max_users = START_USER_LIST_LEN;
    return 1;
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

    while (userlist.users[h] != NULL)
        h = (h + 1) % userlist.max_users;
    userlist.users[h] = user;
    return 1;
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

int get_user(char *name) {
    for (int i = 0; i < userlist.max_users; i++)
       if (userlist.users[i] != NULL)
            if (strncmp(name, userlist.users[i]->name, PLAYER_NAME_LEN) == 0)
                return i;
    return -1; // not found
}
int add_player_to_list(char *nick, uint32_t money) {
    // check if already in if so update
    int i;
    if ((i = get_user(nick)) != -1) {
        userlist.users[i]->money = money;
    }

    // check if >= 66% full
    if (userlist.cur_users >= userlist.max_users * 2 / 3) {
        if (resize_userlist() == -1) {
            return -1;
        }
    }
    // create user
    user_s *new_user = malloc(sizeof(user_s));
    if (new_user == NULL)
        return -1;

    // allocate space for name
    new_user->name = malloc(sizeof(char) * (PLAYER_NAME_LEN + 1));
    if (new_user->name == NULL) {
        free(new_user);
        return -1;
    }
    new_user->name = strncpy(new_user->name, nick, PLAYER_NAME_LEN);
    //add nul terminator to be safe
    new_user->name[PLAYER_NAME_LEN] = '\0';
    // add
    if (hash_user(new_user) == -1) {
        free(new_user->name);
        free(new_user);
        return -1;
    }
    return 1;
}
