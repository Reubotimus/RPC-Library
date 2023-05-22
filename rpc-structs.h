#include "linked-list.h"
#ifndef RPC_STRUCTS_H
#define RPC_STRUCTS_H

/* Handle for remote function */
struct rpc_handle {
	int id;
};

/* Server state */
struct rpc_server {
    int listening_socket;
	Linked_List *functions;
	int number_of_functions;
};

/* Client state */
struct rpc_client {
    int socket_fd;
};

typedef struct {
	struct rpc_server *server;
    int file_descriptor;
} serve_client_args;

#endif
