CC=gcc
CFLAGS= -Wall -g
LDFLAGS = -L. -l:rpc.a

SRCS= linked-list.c rpc-helper-functions.c rpc.c

OBJS=$(SRCS:.c=.o)

rpc.a: $(OBJS)
	ar -rcs rpc.a $(OBJS)

all: clean rpc-client rpc-server


rpc-client: client.o rpc.a
	$(CC) $(CFLAGS) client.o $(LDFLAGS) -o $@

rpc-server: server.o rpc.a
	$(CC) $(CFLAGS) server.o $(LDFLAGS) -o $@

clean:
	rm -f test rpc-server rpc-client *.a *.o