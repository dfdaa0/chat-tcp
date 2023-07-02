CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: server client

server: src/server.c
	mkdir -p bin/server
	$(CC) $(CFLAGS) -o bin/server/server src/server.c

client: src/client.c
	mkdir -p bin/client
	$(CC) $(CFLAGS) -o bin/client/client src/client.c

clean:
	rm -rf bin
