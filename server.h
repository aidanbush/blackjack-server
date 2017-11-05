/* Author: Aidan Bush
 * Assign: Assign 2
 * Course: CMPT 361
 * Date: Oct 26, 17
 * File: server.h
 * Description: deals with running the server
 */

#ifndef SERVER_H
#define SERVER_H

#include "game.h"

extern int verbosity;

extern game_rules rules;
extern game_s game;
extern userlist_s userlist;

/* main server function */
void server();

#endif /* SERVER_H */
