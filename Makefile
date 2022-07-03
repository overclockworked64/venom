CC = gcc
CFLAGS = -g -Wshadow -Wall -Wextra
LDLIBS = -lm

venom:
	$(CC) $(CFLAGS) $(wildcard ./src/*.c) $(LDLIBS)