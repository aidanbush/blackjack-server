/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct 25, 17
 * File: packet.h
 * Description: deals with creating, getting infomation from, and testing
 *  packets.
 */

#ifndef PACKET_H
#define PACKET_H

#include "game.h"

// opcodes
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
#define MESSAGE_LEN     165
#define ACK_LEN         5

// packet checking responce codes
#define P_CHECK_LEN         -1
#define P_CHECK_STATE       -2
#define P_CHECK_DNE         -3
#define P_CHECK_N_ACTIVE    -4
#define P_CHECK_N_CUR       -5
#define P_CHECK_INVAL       -6

#define MAX_PACKET_LEN (STATUS_LEN)

#define ERROR_MSG_LEN   (ERROR_LEN) - 2

// globals needed
extern int verbosity;
extern game_s game;

/* creates the packet for the given active player, and uses the game state data */
uint8_t *create_status_packet(uint8_t player);

/* retrieves the bet from a packet */
uint32_t get_bet(uint8_t *packet);

/* retrieves the opcode from a packet */
uint8_t get_opcode(uint8_t *packet);

/* retrieves the nickname from a connect packet */
char *get_connect_nick(uint8_t *packet);

/* prints out all of a packets bytes */
void print_packet(uint8_t *packet);

/* tests to see if a packet is what was expected
 * returns the player id for the player that send it or one of the P_CHECK
 * return values if it failed a test */
int check_packet(uint8_t *packet, int len, struct sockaddr_storage recv_store, int e_len, game_state e_state);

#endif /* PACKET_H */
