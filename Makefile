CC=gcc
CFLAGS=-Wall -Werror -pedantic-errors

all: chatclient chatserver

chatclient: src/chatclient.c
	$(CC) $(CFLAGS) src/chatclient.c -o chatclient

chatserver: src/chatserver.c
	$(CC) $(CFLAGS) src/chatserver.c -o chatserver

clean:
	rm -f chatclient chatserver