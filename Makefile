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

test_game: test_game.o game.o

test_game.o: test_game.c game.h

blackjack: blackjack.o game.o

blackjack.o: blackjack.c game.h

game.o: game.c game.h

clean:
	$(RM) main *.o test_game
