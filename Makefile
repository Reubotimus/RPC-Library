CC=gcc
CFLAGS= -Wall -g
LDFLAGS = -L. -l:rpc.a

SRCS= linked-list.c rpc-helper-functions.c rpc.c

OBJS=$(SRCS:.c=.o)

rpc.a: $(OBJS)
	ar -rcs rpc.a $(OBJS)

all: clean test client server

test: test.o rpc.a
	$(CC) $(CFLAGS) test.o $(LDFLAGS) -o $@

client: client.o rpc.a
	$(CC) $(CFLAGS) client.o $(LDFLAGS) -o $@

server: server.o rpc.a
	$(CC) $(CFLAGS) server.o $(LDFLAGS) -o $@

clean:
	rm -f test server client *.a *.o