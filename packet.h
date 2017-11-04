/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct 25, 17
 * File: packet.h
 * Description:
 */

#ifndef PACKET_H
#define PACKET_H

#include "game.h"

//opcodes
#define OPCODE_STATUS   0
#define OPCODE_CONNECT  1
#define OPCODE_BET      2
#define OPCODE_STAND    3
#define OPCODE_HIT      4
#define OPCODE_QUIT     5
#define OPCODE_ERROR    6
#define OPCODE_MESSAGE  7
#define OPCODE_ACK      8

// error opcodes
#define ERROR_OP_GEN        0
#define ERROR_OP_MONEY      1
#define ERROR_OP_SEATS      2
#define ERROR_OP_NICK_TAKEN 3
#define ERROR_OP_NICK_INV   4
#define ERROR_OP_TIME       5

// packet lengths
#define STATUS_LEN      320
#define CONNECT_LEN     13
#define BET_LEN         (STATUS_LEN)
#define STAND_LEN       (STATUS_LEN)
#define HIT_LEN         (STATUS_LEN)
#define QUIT_LEN        (STATUS_LEN)
#define ERROR_LEN       142
#define MESSAGE_LEN     5

#define P_CHECK_LEN         -1
#define P_CHECK_STATE       -2
#define P_CHECK_DNE         -3
#define P_CHECK_N_ACTIVE    -4
#define P_CHECK_N_CUR       -5

#define MAX_PACKET_LEN (STATUS_LEN)

#define ERROR_MSG_LEN   (ERROR_LEN) - 2

uint8_t *create_status_packet(uint8_t player);

int validate_packet(uint8_t *packet_1, uint8_t *packet_2);

uint32_t get_bet(uint8_t *packet);

uint8_t get_opcode(uint8_t *packet);

char *get_connect_nick(uint8_t *packet);

//return from player id if correct else -1 through -4 for the error it caught
/*-1 len error, -2 state error, -3 player does not exist, -4 player in not active, -5 not current player*/
int check_packet(uint8_t *packet, int len, struct sockaddr_storage recv_store, int e_len, game_state e_state);

#endif /* PACKET_H */
