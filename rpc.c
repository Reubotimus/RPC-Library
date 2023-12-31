#include "rpc.h"
#include "rpc-helper-functions.h"
#include "linked-list.h"
#include "rpc-structs.h"
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#define LISTEN_QUEUE_LEN 10
#define NONBLOCKING



rpc_server *rpc_init_server(int port) {
	// initialises the server struct
	rpc_server *server = malloc(sizeof(rpc_server));
	server->functions = create_list();
	server->number_of_functions = 0;
    
	server->listening_socket = create_listening_socket(port);

	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
	if (listen(server->listening_socket, LISTEN_QUEUE_LEN) < 0) {
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

	struct sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof client_addr;
	int socket_fd;
	pthread_t new_thread;
	serve_client_args *args;
	while (1) {
		// Accept a connection - blocks until a connection is ready to be accepted
		// Get back a new file descriptor to communicate on
		socket_fd = accept(
			srv->listening_socket, 
			(struct sockaddr*)&client_addr, 
			&client_addr_size
			);
		
		if (socket_fd < 0) {
			perror("accept");
			break;
		}

		// creates new args for new thread
		args = malloc(sizeof(serve_client_args));
		assert(args);
		args->server = srv;
		args->file_descriptor = socket_fd;

		// creates new thread
		if (pthread_create(&new_thread, NULL, &serve_client, args)) {
			perror("pthread create error\n");
			free(args);
			break;
		}

		// detatches thread and continues loop
		pthread_detach(new_thread);
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
	char service[MAX_PORT_STR_LEN];
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
	if (!cl || !name) return NULL;
	
	// send request to server
	char buf[MAX_MSG_LEN];
	sprintf(buf, "%s %s", FIND_CMD_STR, name);
	send(cl->socket_fd, buf, strlen(buf), 0);

	// receive response from server
	int len = recv(cl->socket_fd, buf, MAX_MSG_LEN, 0);
    buf[len] = '\0';

	// get function id from message
	int32_t function_id = ntohl(*((int32_t*)(buf + FUNCTION_MSG_STR_LEN + 1)));

	// returns null if server does not have function
	if (function_id < 0) {
		//printf("function not found\n");
		return NULL;
	}

	// creates and returns the handle
	rpc_handle *new_handle = malloc(sizeof(rpc_handle));
	new_handle->id = function_id;
	//printf("found funct with id: %d\n", new_handle->id);
    return new_handle;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
	if (!cl || !h || !payload) return NULL;

	if ((payload->data2_len == 0 && payload->data2 != NULL) || (payload->data2_len != 0 && payload->data2 == NULL))
		return NULL;

	char message[MAX_MSG_LEN];
	sprintf(message, "%s ", CALL_CMD_STR);
	((int64_t*)(message + (CALL_CMD_STR_LEN + 1)))[0] = (int64_t)h->id;

	// serialise and send data
	serialise_data(
		message     + (CALL_CMD_STR_LEN + 1) + sizeof(int64_t), 
		MAX_MSG_LEN - (CALL_CMD_STR_LEN + 1) - sizeof(int64_t), 
		payload);

	send(
		cl->socket_fd, 
		message,  
		(CALL_CMD_STR_LEN + 1) + 3 * sizeof(int64_t) + payload->data2_len, 
		0);
	
	// get and return response
	int len = recv(cl->socket_fd, message, MAX_MSG_LEN, 0);
	message[len] = '\0';

	if (len == 0) {
		//printf("server crashed\n");
		return NULL;
	}
	if (len == DATA_MSG_STR_LEN) {
		//printf("error occured returning null\n");
		return NULL;
	}
	rpc_data *d = deserialise_data(
		message + DATA_MSG_STR_LEN + 1, 
		len    - (DATA_MSG_STR_LEN + 1));
	if (d == NULL) {
		//printf("error deserialising data\n");
	}
    return d;
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
