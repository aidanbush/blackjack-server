# Author: Aidan Bush
# Assign: Assign 2
# Course: CMPT 361
# Date: Oct. 19, 17
# File: Makefile
# Description: it's a Makefile
SHELL=/bin/bash
CC=gcc
CFLAGS=-Wall -std=c99 -D_POSIX_C_SOURCE=201112L -pedantic -g -D_TEST_MSGS

.PHONY: all clean

all: blackjack

test_game: test_game.o game.o packet.o

test_game.o: test_game.c game.h packet.h server.h

blackjack: blackjack.o game.o server.o packet.o msg.o

blackjack.o: blackjack.c game.h server.h msg.h

game.o: game.c game.h

packet.o: packet.c packet.h game.h

server.o: server.c server.h game.h packet.h msg.h

msg.o: msg.c msg.h game.h

clean:
	$(RM) blackjack *.o test_game
