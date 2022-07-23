all:
	gcc -g -Wall -Werror -pedantic-errors chatclient.c -o chatclient
clean:
	rm -f chatclient chatclient.exe