# Author: Aidan Bush
# Assign: Assign 2
# Course: CMPT 361
# Date: Oct. 19, 17
# File: Makefile
# Description: it's a Makefile

CC=gcc
CFLAGS=-Wall -std=c99 -D_POSIX_C_SOURCE=201112L -pedantic

.PHONY: all clean

all: main

test_game: test_game.o game.o

test_game.o: test_game.c game.h

main: main.o game.o

main.o: main.c game.h

game.o: game.c game.h

clean:
	$(RM) main *.o test_game
