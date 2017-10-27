/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct 23, 17
 * File: packet.c
 * Description:
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

#define VALIDATE_OFF    OFF_SEQ_NUM // start at seqnum
#define VALIDATE_LEN    ((STATUS_LEN) - (VALIDATE_OFF))

#define CONNECT_NICK_OFF 1

static void packet_players(uint8_t *packet) {
    int p_off;
    //for each player
    for (int i = 0; i < (MAX_PLAYERS); i++) {
        if (game.players[i] == NULL)
            continue;
        p_off = (OFF_PLAYER_START) + (OFF_PLAYER_LEN) * i;

        // add player name
        strncpy((char *)(packet + p_off), game.players[i]->nick, PLAYER_NAME_LEN);
        // add player bank
        *((uint32_t *)(packet + p_off + (OFF_PLAYER_BANK))) = game.players[i]->money;// convert to proper endieness
        // add player bet
        *((uint32_t *)(packet + p_off + (OFF_PLAYER_BET))) = game.players[i]->bet;// convert to proper endieness
        // add player cards
        player_s *player = game.players[i];
        for (int j = 0; j < (MAX_NUM_CARDS) && player->cards[j] != 0; j++)
            // dont have to convert since both are uint8_t
            packet[p_off + j] = player->cards[j];
    }
}

static void dealer_cards(uint8_t *packet) {
    // for all shown cards show them set them
    for (int i = 0; i < game.d_shown_cards && i < (MAX_NUM_CARDS); i++)
        packet[OFF_D_CARDS + i] = game.d_cards[i];
}

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
    *((uint8_t *)(packet + (OFF_SEQ_NUM))) = game.seq_num++;

    // min_bet
    *((uint32_t *)(packet + (OFF_MIN_BET))) = htonl(rules.min_bet);

    // active player
    *((uint8_t *)(packet + (OFF_PLAYER))) = player;

    // dealer cards
    dealer_cards(packet);

    // for each player
    packet_players(packet);

    return packet;
}

uint8_t *create_status_packet(uint8_t player) {
    return create_packet(player, (OPCODE_STATUS));
}

int validate_packet(uint8_t *packet_1, uint8_t *packet_2) {
    return memcmp(packet_1 + (VALIDATE_OFF), packet_2 + (VALIDATE_OFF), (VALIDATE_LEN));
}

uint32_t get_bet(uint8_t *packet) {
    if (packet == NULL)
        return 0;
    return (uint32_t) ntohl(*((uint32_t*) (packet + 1)));
}

uint8_t get_opcode(uint8_t *packet) {
    if (packet == NULL)
        return UINT8_MAX;
    return *packet;
}

char *get_connect_nick(uint8_t *packet) {
    return (char *) (packet + CONNECT_NICK_OFF);
}
