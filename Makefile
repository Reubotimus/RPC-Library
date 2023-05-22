CC=gcc
CFLAGS= -Wall -g
LDFLAGS = -L. -l:rpc.a -pthread -l:rpc.a

SRCS= linked-list.c rpc-helper-functions.c rpc.c

OBJS=$(SRCS:.c=.o)

rpc.a: $(OBJS)
	ar -rcs rpc.a $(OBJS)

all: clean rpc-client rpc-server


rpc-client: rpc.a
	$(CC) $(CFLAGS) client.a $(LDFLAGS) -o $@

rpc-server: rpc.a
	$(CC) $(CFLAGS) server.a $(LDFLAGS) -o $@

clean:
	rm -f test rpc-server rpc-client rpc.a *.o *.txt