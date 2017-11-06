/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct. 30, 17
 * File: msg.h
 * Description:
 */

#ifndef MSG_H
#define MSG_H

#include <stdint.h>

#include "game.h"

#define MSG_BUF_LEN     5
#define MSG_LEN         165

typedef struct msg_list {
    uint8_t *msgs[MSG_BUF_LEN];
    int cur_pos;// start
    int num_msgs;
    int size;
    uint32_t serv_seq;
} msg_list;

// list of server seq numbers
typedef struct msg_ack_list {
    //multi dimensional array for acks, 0 is recived
    uint32_t ack_list[MAX_PLAYERS][MSG_BUF_LEN];
} msg_ack_list;

void init_msg_queue();

void free_msg_queue();

int add_msg(uint8_t *msg);

void recive_ack(uint8_t *msg, int player);

int msg_acked(int msg, int player);

#endif /* MSG_H */
