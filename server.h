/*
	Copyright 2014 Shipping Soon. All Rights Reserved.
*/

#define BACKLOG 10
#define KBYTE 1024
#define DEBUG_MODE
#define OK 200
#define CREATED 201
#define ACCEPTED 202
#define PARTIAL_INFORMATION 203
#define NO_RESPONSE 204
#define BAD_REQUEST 400
#define UNAUTHORIZED 401
#define PAYMENT_REQUIRED 402
#define FORBIDDEN 403
#define NOT_FOUND 404
#define INTERNAL_ERROR 500
#define NOT_IMPLEMENTED 501
#define CONFIG_FILE_PATH "etc/cserver/cserver.conf"
#define DEFAULT_PORT "6301"
#define DEFAULT_SERVERNAME "cServer"
#define DEFAULT_DIRECTORY "www"
#define DEFAULT_LOG_DIRECTORY "var/log"
#define DEFAULT_DIRECTORY_INDEX "index.html"
#define DEFAULT_VERSION "v0.04"
#define FREE(p)	\
	free(p);	\
	(p) = NULL; 
#define CONCAT(x) # x
#define CAT(x) CONCAT(x)

/******* http.h *******/

//HTTP status
struct http_info {
	int code;
	char message[100];
};

//Retrieve and format the local time.
void get_local_time(char *ptime, size_t max);

//Adds a new entry in the HTTP response.
int build_response(char *buffer, int idx, const char *format, ...);

//Sets the HTTP response message.
void http_status(char *message, int code);

/******* server.h *******/

//Network sockets.
struct socket {
	char buffer[KBYTE], ip[INET_ADDRSTRLEN];
	int sd;
};

//Holds the server's configuration data.
struct server_info {
	char path[PATH_MAX], log[PATH_MAX], port[6], name[255], version[10], index[NAME_MAX];
	struct socket net;
};

//The client's information.
struct client_info {
	char request[PATH_MAX], agent[300];
	struct socket net;
};

//Initiates the server and blocks until a connection has been established.
int server(struct server_info *serv, struct client_info *client);

//Receives an HTTP request.
void request(struct server_info *serv, struct client_info *client);

//Processes the HTTP request and builds a response.
void handle_request(struct server_info *serv, struct client_info *client);

//Terminate the program and display debugging information.
void die(const char * s, int sock, struct addrinfo * res);

/******* config.h *******/

//Loads the contents of a file into memory. Note: does not free up memory.
long load_file(const char *file_path, char **buffer);

//Set default values for the configuration struct.
void config_init(struct server_info *serv);

//Parse the configuration file's entries line by line.
void parse_config(struct server_info *serv, char *file_data, size_t max, const char *delimiters);

//Logs server and client information.
void log_message(const char *file_path, char *buffer);
