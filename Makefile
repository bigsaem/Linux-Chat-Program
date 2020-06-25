CC = gcc
FLAG = -Wall
CFLAGS  = -Wall -pthread

all: server client

server: server.c
	$(CC) $(FLAG) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c



