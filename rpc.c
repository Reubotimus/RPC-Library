#include "rpc.h"
#include "rpc-helper-functions.h"
#include "linked-list.h"
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

struct rpc_server {
    int socket_fd;
	Linked_List *functions;
	int number_of_functions;
};

rpc_server *rpc_init_server(int port) {
    
	int socket_fd = create_listening_socket(port);
	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
	if (listen(socket_fd, 5) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	// initialises the server struct
	struct rpc_server *server = malloc(sizeof(struct rpc_server));
	server->functions = create_list();
	server->number_of_functions = 0;

	// Accept a connection - blocks until a connection is ready to be accepted
	// Get back a new file descriptor to communicate on
	struct sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof client_addr;
	server->socket_fd =
		accept(socket_fd, (struct sockaddr*)&client_addr, &client_addr_size);
	if (server->socket_fd < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	char ip[INET_ADDRSTRLEN];
	// Print ipv4 peer information (can be removed)
	getpeername(server->socket_fd, (struct sockaddr*)&client_addr, &client_addr_size);
	inet_ntop(client_addr.sin_family, &client_addr.sin_addr, ip,
			  INET_ADDRSTRLEN);
	port = ntohs(client_addr.sin_port);
	printf("new connection from %s:%d on socket %d\n", ip, port, server->socket_fd);
    
	// close unused socket and returns server
	close(socket_fd);
	return server;
}

// struct used to encapsulate a function held by the server
typedef struct {
	char *name;
	int id;
} function;

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
	if (srv == NULL || name == NULL || handler == NULL) return -1;

	// if there is a function with given name, removes it from list
	Node *node = srv->functions->head, *previous = NULL;
	while (node != NULL) {
		if (strcmp(((function*)(node->data))->name, name) == 0) {
			free(node->data);
			remove_node(srv->functions, node, previous);
		}
		previous = node;
		node = node->next;
	}

	function *new_function = malloc(sizeof(function));
	new_function->id = srv->number_of_functions++;
	new_function->name = strdup(name);

	insert_at_foot(srv->functions, new_function);
    return new_function->id;
}

void rpc_serve_all(rpc_server *srv) {

}

struct rpc_client {
    /* Add variable(s) for client state */
    int socket_fd;
};

struct rpc_handle {
    /* Add variable(s) for handle */
};

rpc_client *rpc_init_client(char *addr, int port) {
    int s;
	struct addrinfo hints, *servinfo, *rp;

	// Create address
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	// convert port into string for getaddrinfo function
	char service[6];
	sprintf(service, "%d", port);

	// Get addrinfo of server
	s = getaddrinfo(addr, service, &hints, &servinfo);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Connect to first valid result
	struct rpc_client *client = malloc(sizeof(struct rpc_client));
	for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
		client->socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (client->socket_fd == -1)
			continue;

		if (connect(client->socket_fd, rp->ai_addr, rp->ai_addrlen) != -1)
			break; // success

		close(client->socket_fd);
	}

	if (rp == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(servinfo);

    return client;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {
    return NULL;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    return NULL;
}

void rpc_close_client(rpc_client *cl) {

}

void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}
