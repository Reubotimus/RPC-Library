#include "rpc.h"

// creates a listening socket and returns the file descriptor
int create_listening_socket(int port);

// used to represent client request types
enum REQUEST_TYPE{FIND_REQUEST, CALL_REQUEST, INVALID_REQUEST};

// returns the request type from a message recieved from the client
enum REQUEST_TYPE get_request_type(char *message);

// handle the FIND request from client
void handle_find(rpc_server *server, char *message);

// handle the CALL request from client
void handle_call(rpc_server *server, char *message);

// serialises the data into the byte stream `send_data`
void serialise_data(void *send_data, int send_data_length, rpc_data* data);

// deserialises the data from byte stream returns a malloced data
rpc_data *deserialise_data(void *serialised_data);

void print_bits(void *bytes, size_t size);

// struct used to encapsulate a function held by the server
typedef struct {
	char *name;
	int id;
	rpc_handler handler;
} function;