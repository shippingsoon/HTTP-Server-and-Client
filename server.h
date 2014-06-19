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
#define DEFAULT_DIRECTORY "wwwxxx"
#define DEFAULT_VERSION "v0.04"
#define FREE(p)	\
	free(p);	\
	(p) = NULL; 
#define CONCAT(x) # x





/******* http *******/

//Retrieve and format the local time.
void get_local_time(char *ptime, size_t max);

//Adds a new entry in the HTTP response.
int build_response(char *buffer, int idx, const char *format, ...);

/******* server *******/

//Holds the server's configuration data.
struct server_info {
	char path[PATH_MAX], port[6], name[255], version[10];
	int sd;
};

//The client's information.
struct client_info {
	char request[KBYTE], ip[INET_ADDRSTRLEN];
	int sd;
};

//Initiates the server.
int server(struct server_info *serv, struct client_info *client);

//Builds the HTTP response.
void response(struct server_info *serv, struct client_info *client);

//Terminate the program and display debugging information.
void die(const char * s, int sock, struct addrinfo * res);

/******* config *******/

//Loads the contents of a file into memory. Note: does not free up memory.
long load_file(const char *file_path, char **buffer);

//Set default values for the configuration struct.
void config_init(struct server_info *serv);

//Parse the configuration file's entries line by line.
void parse_config(struct server_info *serv, char *file_data, size_t max, const char *delimiters);
