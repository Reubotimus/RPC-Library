#include "rpc.h"

#define MAX_PORT_STR_LEN 6

#define MAX_MSG_LEN 100022
#define MAX_FUNCTION_NAME_LEN 1000

#define FIND_CMD_STR "FIND"
#define FIND_CMD_STR_LEN 4

#define CALL_CMD_STR "CALL"
#define CALL_CMD_STR_LEN 4

#define DATA_MSG_STR "DATA"
#define DATA_MSG_STR_LEN 4

#define FUNCTION_MSG_STR "FUNCTION"
#define FUNCTION_MSG_STR_LEN 8

// creates a listening socket and returns the file descriptor
int create_listening_socket(int port);

// used to represent client request types
enum REQUEST_TYPE{FIND_REQUEST, CALL_REQUEST, INVALID_REQUEST};

// returns the request type from a message recieved from the client
enum REQUEST_TYPE get_request_type(char *message);

// handle the FIND request from client
void handle_find(rpc_server *server, char *message);

// handle the CALL request from client
void handle_call(rpc_server *server, char *message, int message_len);

// serialises the data into the byte stream `send_data`
void serialise_data(void *send_data, int send_data_length, rpc_data* data);

// deserialises the data from byte stream returns a malloced data
rpc_data *deserialise_data(void *serialised_data, int array_len);

// handles all requests from client until client closes
void serve_client(rpc_server *srv);

// struct used to encapsulate a function held by the server
typedef struct {
	char *name;
	int id;
	rpc_handler handler;
} function;