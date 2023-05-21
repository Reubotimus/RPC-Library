#include "rpc-helper-functions.h"
#include "linked-list.h"
#include "rpc.h"
#include "rpc-structs.h"

#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

// reverses byte order of int64_t if host is not in network byte order
int64_t reverse_byte_order(int64_t number) {
	// if already in network byte order, return number
	if (1 == htonl(1)) {
		return number;
	}

	int8_t result[sizeof(int64_t)];
	int number_address = sizeof(int64_t) - 1;
	for (int i = 0; i < sizeof(int64_t); i++) {
		result[i] = ((int8_t*)(&number))[number_address--];
	}
	
	return *((int64_t*)result);
}



int create_listening_socket(int port) {
	int re, s, sockfd;
	struct addrinfo hints, *res;


	// Create address we're going to listen on (with given port number)
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // Connection-mode byte streams
	hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept

	// convert port into string for getaddrinfo function
	char service[6];
	sprintf(service, "%d", port);
	
	// node (NULL means any interface), service (port), hints, res
	s = getaddrinfo(NULL, service, &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	// Bind address to the socket
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(res);

	return sockfd;
}

// returns the request type from a message recieved from the client
enum REQUEST_TYPE get_request_type(char *message) {
	// copy request header
	char buf[100];
	int i;
	for (i = 0; message[i] != '\0' && message[i] != ' '; i++) {
		buf[i] = message[i];
	}
	buf[i] = '\0';

	// returns appropriate request type
	if (!strcmp(buf, "FIND")) return FIND_REQUEST;
	if (!strcmp(buf, "CALL")) return CALL_REQUEST;
	return INVALID_REQUEST;
}

// private helper function finds function from linked list
int search_function_list(Linked_List *list, char *name) {
	Node *node = list->head;
	while (node != NULL) {
		if (strcmp(name, ((function*)(node->data))->name) == 0) {
			return ((function*)(node->data))->id;
		}
		node = node->next;
	}
	return -1;
}

// handle the FIND request from client
void handle_find(rpc_server *server, char *message) {
	char function_name[100], return_string[100];
	sscanf(message, "FIND %s", function_name);
	sprintf(return_string, "FUNCTION %d", search_function_list(server->functions, function_name));
	send(server->socket_fd, return_string, strlen(return_string), 0);
	//printf("FIND request response: %s\n", return_string);
}

// handle the CALL request from client
void handle_call(rpc_server *server, char *message) {
	// gets inputs from message
	int64_t function_id = ((int64_t*)(message + 5))[0];
	rpc_data *input_data = deserialise_data(message + 5 + 8);
	char return_string[1000];
	//printf("id: %ld d1: %d d2_len: %ld d2: %s\n", function_id, input_data->data1, input_data->data2_len, (char*)input_data->data2);

	// finds appropriate function
	function *funct = NULL;
	Node *current = server->functions->head;
	while (current != NULL) {
		if (((function*)(current->data))->id == function_id) {
			funct = (function*)(current->data);
			break;
		}
		current = current->next;
	}

	// if function null sends return error
	if (funct == NULL) {
		sprintf(return_string, "DATA NULL");
		send(server->socket_fd, return_string, strlen(return_string), 0);
	}

	// gets return data and sends it to client
	rpc_data *return_data = (funct->handler)(input_data);
	assert(return_data != NULL);
	sprintf(return_string, "DATA ");
	serialise_data(return_string + 5, 1000 - 5, return_data);

	send(server->socket_fd, return_string, 5 + 2*8 + return_data->data2_len, 0);
}

// serialises the data into the byte stream `serialised_data`
void serialise_data(void *serialised_data, int serialised_data_length, rpc_data* data) {
	assert(serialised_data_length >= 8 + 8 + data->data2_len);

	int64_t d1 = data->data1;
	int64_t d2_len = data->data2_len;

	((int64_t*)serialised_data)[0] = reverse_byte_order(d1);
	((int64_t*)serialised_data)[1] = reverse_byte_order(d2_len);

	if (d2_len == 0) return;
	
	int i;
	for (i = 0; i < data->data2_len; i++) {
		((char*)(serialised_data + 2 * sizeof(int64_t)))[i] = ((char*)data->data2)[i];
	}
}

// deserialises the data from byte stream returns a malloced data
rpc_data *deserialise_data(void *serialised_data) {
	rpc_data *return_data = malloc(sizeof(rpc_data));

	return_data->data1 = reverse_byte_order(((int64_t*)serialised_data)[0]);
	return_data->data2_len = reverse_byte_order(((int64_t*)serialised_data)[1]);
	
	if (return_data->data2_len <= 0) {
		return_data->data2 = NULL;
		return return_data;
	}
	// otherwise
	return_data->data2 = malloc(return_data->data2_len);
	memcpy(return_data->data2, serialised_data + 16, return_data->data2_len);
	return return_data;
}
