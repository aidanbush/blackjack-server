/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct 23, 17
 * File: packet.c
 * Description: deals with creating, getting information from and testing
 *  packets.
 */

/* standard libraries */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h> // make sure used

/* system libraries */
#include <arpa/inet.h>

/* project includes */
#include "game.h"
#include "packet.h"

// offset defines
#define OFF_OPCODE          0
#define OFF_RESPONCE_CODE   (OFF_OPCODE) + 1
#define OFF_SEQ_NUM         (OFF_RESPONCE_CODE) + 4
#define OFF_MIN_BET         (OFF_SEQ_NUM) + 2
#define OFF_PLAYER          (OFF_MIN_BET) + 4
#define OFF_D_CARDS         (OFF_PLAYER) + 1
// player offsets
#define OFF_PLAYER_START    (OFF_D_CARDS) + 21
#define OFF_PLAYER_BANK     12
#define OFF_PLAYER_BET      (OFF_PLAYER_BANK) + 4
#define OFF_PLAYER_CARDS    (OFF_PLAYER_BET) + 4
#define OFF_PLAYER_LEN      41

#define VALIDATE_OFF    OFF_MIN_BET // after seqnum
#define VALIDATE_LEN    ((STATUS_LEN) - (VALIDATE_OFF))

#define CONNECT_NICK_OFF 1

/* adds the player state information to a given packet */
static void packet_players(uint8_t *packet) {
    int p_off;
    // for each player
    for (int i = 0; i < (MAX_PLAYERS); i++) {
        if (game.players[i] == NULL)
            continue;
        p_off = (OFF_PLAYER_START) + (OFF_PLAYER_LEN) * i;

        // add player name
        strncpy((char *)(packet + p_off), game.players[i]->nick, PLAYER_NAME_LEN);
        // add player bank
        *((uint32_t *)(packet + p_off + (OFF_PLAYER_BANK))) = htonl(game.players[i]->money);
        // add player bet
        *((uint32_t *)(packet + p_off + (OFF_PLAYER_BET))) = htonl(game.players[i]->bet);
        // add player cards
        player_s *player = game.players[i];
        for (int j = 0; j < (MAX_NUM_CARDS) && player->cards[j] != 0; j++)
            packet[p_off + (OFF_PLAYER_CARDS) + j] = player->cards[j];
    }
}

/* adds the dealers cards to the given packet */
static void dealer_cards(uint8_t *packet) {
    // for all shown cards show them set them
    for (int i = 0; i < game.d_shown_cards && i < (MAX_NUM_CARDS); i++)
        packet[OFF_D_CARDS + i] = game.d_cards[i];
}

/* creates a packet for the given player and opcode */
static uint8_t *create_packet(uint8_t player, uint8_t opcode) {
    uint8_t *packet = malloc(sizeof(uint8_t) * (STATUS_LEN));
    if (packet == NULL)
        return NULL;
    uint8_t *tmp_packet = memset(packet, 0, (STATUS_LEN));
    if (tmp_packet == NULL) {
        free(packet);
        return NULL;
    }
    packet = tmp_packet;
    // opcode
    *packet = opcode;

    // Seqnum
    *((uint16_t *)(packet + (OFF_SEQ_NUM))) = htons(game.seq_num++);

    // min_bet
    *((uint32_t *)(packet + (OFF_MIN_BET))) = htonl(rules.min_bet);

    // active player
    *((uint8_t *)(packet + (OFF_PLAYER))) = player;

    // dealer cards
    dealer_cards(packet);

    // player information
    packet_players(packet);

    return packet;
}

/* creates and returns the packet for the current state and given player */
uint8_t *create_status_packet(uint8_t player) {
    return create_packet(player, (OPCODE_STATUS));
}

/* prints the name from a packet, must be given the starting point */
static void print_packet_name(uint8_t *packet, int start) {
    printf("pn:");
    for (int i = 0; i < PLAYER_NAME_LEN; i++)
        printf("%x", *(packet + start + i));
    printf(" ");
}

/* prints the cards from a packet, must be given the starting point */
static void print_cards(uint8_t *packet, int start) {
    printf("dc");
    for (int i = 0; i < (MAX_NUM_CARDS); i++) {
        printf(" %1x", *(packet + start + i));
    }
    printf("\n");
}

/* prints off all the players for a given player*/
static void print_packet_players(uint8_t *packet) {
    int p_off;
    // for all players
    for (int i = 0; i < (MAX_PLAYERS); i++) {
        p_off = (OFF_PLAYER_START) + (OFF_PLAYER_LEN) * i;
        // print name
        print_packet_name(packet, p_off);
        // print bank
        printf("bn:%4x", *(uint32_t *)(packet + p_off + OFF_PLAYER_BANK));
        // print bet
        printf("bt:%4x\n", *(uint32_t *)(packet + p_off + OFF_PLAYER_BET));
        // print cards
        print_cards(packet, p_off + OFF_PLAYER_CARDS);
    }
}

/* prints out the given packet */
void print_packet(uint8_t *packet) {
    // opcode
    printf("op:%1x ", *packet);
    // responce args
    printf("ra:%4x ", *(uint32_t *)(packet + 1));
    // seq_num
    printf("sq:%2x ", *(uint16_t *)(packet + OFF_SEQ_NUM));
    // min bet
    printf("mb:%4x ", *(uint32_t *)(packet + OFF_MIN_BET));
    // active player
    printf("ap:%1x\n", *(packet + OFF_PLAYER));
    // dealer cards
    print_cards(packet, (OFF_D_CARDS));
    // print players
    print_packet_players(packet);
}

/* compairs the given packet to what the server is currently expecting*/
static int validate_packet(uint8_t *packet_1) {
    // generate packet
    uint8_t *packet_2 = create_status_packet(game.cur_player + 1);
    int cmp = memcmp(packet_1 + (VALIDATE_OFF), packet_2 + (VALIDATE_OFF), (VALIDATE_LEN));
    free(packet_2);
    return cmp;
}

/* returns the bet from a given packet, or returns 0 if the packet is NULL */
uint32_t get_bet(uint8_t *packet) {
    if (packet == NULL)
        return 0;
    return (uint32_t) ntohl(*((uint32_t*) (packet + 1)));
}

/* returns the opcode from a given packet, or returns UINT8_MAX if the packet is
 * NULL */
uint8_t get_opcode(uint8_t *packet) {
    if (packet == NULL)
        return UINT8_MAX;
    return *packet;
}

/* returns the start of the character name string for a given packet, or returns
 * NULL if the packet is NULL */
char *get_connect_nick(uint8_t *packet) {
    if (packet == NULL)
        return NULL;
    return (char *) (packet + CONNECT_NICK_OFF);
}

/* tests the given packet and returns the a failing code or the player id for
 *  who send the packet*/
int check_packet(uint8_t *packet, int len, struct sockaddr_storage recv_store, int e_len, game_state e_state) {
    // check length
    if (len != e_len)
        return P_CHECK_LEN;

    // check state
    if (game.state != e_state)
        return P_CHECK_STATE;

    // get player
    int p = get_player_sock(recv_store);
    if (p == -1)
        return P_CHECK_DNE;

    // check if player is active
    if (game.players[p]->active != PLAYER_A_ACTIVE)
        return P_CHECK_N_ACTIVE;

    // if current player
    if (p != game.cur_player)
        return P_CHECK_N_CUR;

    // if the packet is what was expected
    if (validate_packet(packet) != 0)
        return P_CHECK_INVAL;

    return p;
}
