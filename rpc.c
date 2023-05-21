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
#include <assert.h>
#define LISTEN_QUEUE_LENGTH 5



rpc_server *rpc_init_server(int port) {
	// initialises the server struct
	rpc_server *server = malloc(sizeof(rpc_server));
	server->functions = create_list();
	server->number_of_functions = 0;
    
	server->listening_socket = create_listening_socket(port);

	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
	if (listen(server->listening_socket, LISTEN_QUEUE_LENGTH) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	return server;
}



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
	new_function->handler = handler;

	insert_at_foot(srv->functions, new_function);
    return new_function->id;
}

void rpc_serve_all(rpc_server *srv) {
	int len;
	char buf[100];
	struct sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof client_addr;
	while (1) {
		// Accept a connection - blocks until a connection is ready to be accepted
		// Get back a new file descriptor to communicate on
		srv->socket_fd =
			accept(srv->listening_socket, (struct sockaddr*)&client_addr, &client_addr_size);
		if (srv->socket_fd < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		// while connected handle all requests
		while (1) {
			len = recv(srv->socket_fd, buf, 100, 0);
			buf[len] = '\0';

			// connection terminated
			if (len == 0) {
				close(srv->socket_fd);
				break;
			}

			switch(get_request_type(buf)) {
				case FIND_REQUEST:
					handle_find(srv, buf);
					break;
				case CALL_REQUEST:
					handle_call(srv, buf);
					break;
				case INVALID_REQUEST:
					printf("invalid request\n");
					break;
			}
		}
	}
}

rpc_client *rpc_init_client(char *addr, int port) {
    int s;
	struct addrinfo hints, *servinfo, *rp;

	// Create address
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
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
	// send request to server
	char buf[100];
	sprintf(buf, "FIND %s", name);
	send(cl->socket_fd, buf, strlen(buf), 0);

	// receive response from server
	int len = recv(cl->socket_fd, buf, 100, 0);
    buf[len] = '\0';

	// converts response into a function handle
	rpc_handle *new_handle = malloc(sizeof(rpc_handle));
	if (sscanf(buf, "FUNCTION %d", &(new_handle->id)) != 1) {
		printf("given wrong message type\n");
		exit(EXIT_FAILURE);
	}

    return new_handle;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
	char message[1000];
	sprintf(message, "CALL ");
	((int64_t*)(message + 5))[0] = (int64_t)h->id;
	serialise_data(message + 5 + 8, 1000 - 5 - 8, payload);

	send(cl->socket_fd, message,  5 + 8 + 8 + 8 + payload->data2_len, 0);
	int len = recv(cl->socket_fd, message, 1000, 0);
	message[len] = '\0';
	
    return deserialise_data(message + 5);
}

void rpc_close_client(rpc_client *cl) {
	close(cl->socket_fd);
	free(cl);
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
