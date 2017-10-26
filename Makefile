# Author: Aidan Bush
# Assign: Assign 2
# Course: CMPT 361
# Date: Oct. 19, 17
# File: Makefile
# Description: it's a Makefile

CC=gcc
CFLAGS=-Wall -std=c99 -D_POSIX_C_SOURCE=201112L -pedantic -g

.PHONY: all clean

all: blackjack

test_game: test_game.o game.o packet.o

test_game.o: test_game.c game.h packet.h

blackjack: blackjack.o game.o

blackjack.o: blackjack.c game.h

game.o: game.c game.h

packet.o: packet.c packet.h game.h

clean:
	$(RM) blackjack *.o test_game
