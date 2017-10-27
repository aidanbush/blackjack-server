/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct 25, 17
 * File: packet.h
 * Description:
 */

#ifndef PACKET_H
#define PACKET_H

#define STATUS_LEN 320

uint8_t *create_status_packet(uint8_t player);

int validate_packet(uint8_t *packet_1, uint8_t *packet_2);

uint32_t get_bet(uint8_t *packet);

uint8_t get_opcode(uint8_t *packet);

#endif /* PACKET_H */
