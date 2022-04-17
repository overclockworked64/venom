CC = gcc
CFLAGS = -g -Wshadow -Wall

venom:
	$(CC) $(CFLAGS) $(wildcard ./src/*.c)