CC = gcc
CFLAGS = -g -Wshadow -Wall -Wextra

venom:
	$(CC) $(CFLAGS) $(wildcard ./src/*.c)