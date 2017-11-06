/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct. 30, 17
 * File: msg.c
 * Description:
 */

/* standard libraries */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* system libraries */
#include <arpa/inet.h>

/* project includes */
#include "msg.h"
#include "game.h"

#define OFF_MSG_SERV_ACK    5
#define OFF_ACK_SEQ         1

msg_list msg_queue;
msg_ack_list msg_ack_queue;

/* inits the msg_queue and msg_ack_queu*/
void init_msg_queue() {
    for (int i = 0; i < MSG_BUF_LEN; i++) {
        msg_queue.msgs[i] = NULL;
    }
    msg_queue.cur_pos = 0;
    msg_queue.num_msgs = 0;
    msg_queue.serv_seq = 1;
    msg_queue.size = MSG_BUF_LEN;

    //set all acks to 0
    for (int i = 0; i < MAX_PLAYERS; i++)
        for (int j = 0; j < MSG_BUF_LEN; j++)
            msg_ack_queue.ack_list[i][j] = 0;
}

void free_msg_queue() {
    //free the messages
    for (int i = 0; i < MSG_BUF_LEN; i++) {
        free(msg_queue.msgs[i]);
        msg_queue.msgs[i] = NULL;
    }
    //set all acks to 0
    for (int i = 0; i < MAX_PLAYERS; i++)
        for (int j = 0; j < MSG_BUF_LEN; j++)
            msg_ack_queue.ack_list[i][j] = 0;
}

static void set_server_ack(uint8_t *msg) {
    msg_queue.serv_seq++;
    if (msg_queue.serv_seq == 0)//since 0 if to notify that an ack was recieved
        msg_queue.serv_seq++;
    *(uint32_t *)(msg + OFF_MSG_SERV_ACK) = htonl(msg_queue.serv_seq);
}

static void remove_last_msg() {
    int pos = msg_queue.cur_pos + msg_queue.num_msgs;

    if (msg_queue.msgs[pos] == NULL)
        return;

    //free msg
    free(msg_queue.msgs[pos]);
    msg_queue.msgs[pos] = NULL;

    //set all acks to 0
    for (int i = 0; i < MAX_PLAYERS; i++) {
        msg_ack_queue.ack_list[i][pos] = 0;
    }
}

// adds a message to the queue
int add_msg(uint8_t *msg) {
    //copy over
    uint8_t *msg_cpy = NULL;
    msg_cpy = memcpy(msg_cpy, msg, MSG_LEN);

    //update server sequence number
    set_server_ack(msg_cpy);

    //if need to remove messages
    if (msg_queue.num_msgs >= MSG_BUF_LEN) {
        remove_last_msg();
    }

    //update current position
    msg_queue.cur_pos = (msg_queue.cur_pos + msg_queue.num_msgs) % MSG_BUF_LEN;

    //add to queue
    msg_queue.msgs[msg_queue.cur_pos] = msg_cpy;
    //update number of messages
    msg_queue.num_msgs++;

    //set acks
    for (int i = 0; i < MAX_PLAYERS; i++) {
        msg_ack_queue.ack_list[i][msg_queue.cur_pos] = msg_queue.serv_seq;
    }
    return 1;
}

static uint32_t get_seq_ack(uint8_t *msg) {
    return ntohl(*(uint32_t *)(msg + OFF_ACK_SEQ));
}

void recive_ack(uint8_t *msg, int player) {
    uint32_t seq = get_seq_ack(msg);

    for (int i = 0; i < MSG_BUF_LEN; i++) {
        if (msg_ack_queue.ack_list[player][i] == seq) {
            msg_ack_queue.ack_list[player][i] = 0;//set to recived
            //dont need to check if it was the last beause the messages can just wont be resent
            break;
        }
    }
}

//return 1 if the message has been acked by the give player otherwise return 0
int msg_acked(int msg, int player) {
    //if valid msg and player
    if (msg < 0|| msg >= MSG_BUF_LEN || player < 0 || player >= MAX_PLAYERS)
        return 0;
    //check if acked
    if (msg_ack_queue.ack_list[player][msg] == 0)
        return 1;
    return 0;
}
