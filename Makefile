CC=gcc
CPPFLAGS= -Wall 
LDFLAGS =



debug ?= 1 
ifeq ($(debug),1) 
	CPPFLAGS= -Wall -Werror -g
endif

all: rpc.o linked-list.o rpc-helper-functions.o

rpc.o: 
	$(CC) -c $(CPPFLAGS) rpc.c -o rpc.o

linked-list.o: linked-list.c
	$(CC) -c $(CPPFLAGS) linked-list.c -o linked-list.o

rpc-helper-functions.o: rpc-helper-functions.c
	$(CC) -c $(CPPFLAGS) rpc-helper-functions.c -o rpc-helper-functions.o

test: rpc.o linked-list.o rpc-helper-functions.o test.c
	$(CC) $(CPPFLAGS) test.c rpc.o linked-list.o rpc-helper-functions.o -o test

client: rpc.o linked-list.o rpc-helper-functions.o client.c
	$(CC) $(CPPFLAGS) client.c rpc.o linked-list.o rpc-helper-functions.o -o client

server: rpc.o linked-list.o rpc-helper-functions.o server.c
	$(CC) $(CPPFLAGS) server.c rpc.o linked-list.o rpc-helper-functions.o -o server

clean:
	rm -f test server client *.o