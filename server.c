/*
	Copyright 2014 Shipping Soon. All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>
#include "server.h"

int main(void)
{
	char *file_data;
	long file_size;
	struct server_info serv;
	struct client_info client;
	
	//Set default values for our server's settings.
	config_init(&serv);
	//Load the configuration file into memory.
	if ((file_size = load_file(CONFIG_FILE_PATH, &file_data)) != 0)
		//Parse the configuration file line by line.
		parse_config(&serv, file_data, file_size, "\n");
	//Free the data.
	FREE(file_data);
	//Initiate the server and block until a connection has been established.
	server(&serv, &client);
	//Process HTTP request.
	request(&serv, &client);
	return EXIT_SUCCESS;
}

//Loads the contents of a file into memory. Note: does not free up memory.
long load_file(const char *file_path, char **buffer)
{
	FILE *file;
	size_t result;
	long file_size;
	
	file_size = 0;
	//Open the file for reading.
	if ((file = fopen(file_path, "rb")) != NULL) {
		//Get the file's size.
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		rewind(file);
		//Dynamically allocate memory.
		if ((*buffer = (char *) malloc(sizeof(char) * file_size)) == NULL)
			fputs("malloc() error allocating memory", stderr);
		//Load the file into memory.
		if ((result = fread(*buffer, sizeof(char), file_size, file)) != file_size)
			fputs("fread() error", stderr);
		//Close the file.
		fclose(file);
	}
	return file_size;
}

//Set default values for the configuration struct.
void config_init(struct server_info *serv)
{
	//Set defaults values.
	strncpy(serv->path, DEFAULT_DIRECTORY, PATH_MAX);
	strncpy(serv->index, DEFAULT_DIRECTORY_INDEX, NAME_MAX);
	strncpy(serv->port, DEFAULT_PORT, 6);
	strncpy(serv->name, DEFAULT_SERVERNAME, 255);
	strncpy(serv->version, DEFAULT_VERSION, 10);
}

//Parse the configuration file's entries line by line.
void parse_config(struct server_info *serv, char *file_data, size_t max, const char *delimiters)
{
	char *token, data[max];
	
	//Set defaults
	strncpy(data, file_data, max);
	for (token = strtok(data, delimiters); token != NULL; token = strtok(NULL, delimiters)) {
		//Get the server name.
		if (strstr(token, "ServerName") != NULL)
			sscanf(token, "%*s %255s", serv->name);
		//Get the port number.
		else if (strstr(token, "Port") != NULL)
			sscanf(token, "%*s %6s", serv->port);
		//Get the public HTML directory.
		else if (strstr(token, "Directory") != NULL)
			sscanf(token, "%*s %"CAT(PATH_MAX)"s", serv->path);
		//Get the server's version.
		else if (strstr(token, "Version") != NULL)
			sscanf(token, "%*s %10s", serv->version);
		//Get the default directory file.
		else if (strstr(token, "Index") != NULL)
			sscanf(token, "%*s %"CAT(NAME_MAX)"s", serv->index);
	}
}

//Initiates the server and blocks until a connection has been established.
int server(struct server_info *serv, struct client_info *client)
{
	struct addrinfo hints, *res, *ai;
	struct sockaddr_in dest;
	int error, yes, sock_size;
	
	sock_size =  sizeof(struct sockaddr_in);
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	yes = 1;
	//Use getaddrinfo() to retrieve a linked list of addrinfo structs.
	if ((error = getaddrinfo(NULL, serv->port, &hints, &res)) != 0) {
		#ifdef DEBUG_MODE
		fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(error));
		#endif
		return EXIT_FAILURE;
	}
	//Loop through our linked list of addrinfo structs.
	for (ai = res; ai != NULL; ai = ai->ai_next) {
		//Retrieve the socket descriptor.
		if ((serv->net.sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1)
			continue;
		break;
	}
	//Allow our socket to forcibly bind to a port.
	if (setsockopt(serv->net.sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		die("setsockopt() error", serv->net.sd, res);
	//Bind the socket to a port.
	if ((bind(serv->net.sd, res->ai_addr, res->ai_addrlen)) == -1)
		die("bind() error", serv->net.sd, res);
	//This dynamically allocated addrinfo is no longer needed.
	freeaddrinfo(res);
	//Make our socket descriptor listen on a port.
	if ((listen(serv->net.sd, BACKLOG)) == -1)
		die("listen() error", serv->net.sd, NULL);
	//Accept an incoming connection.
	if ((client->net.sd = accept(serv->net.sd, (struct sockaddr *) &dest, &sock_size)) == -1)
		die("accept() error", serv->net.sd, NULL);
	else
		strncpy(client->net.ip, inet_ntoa(dest.sin_addr), INET_ADDRSTRLEN);
	printf("Server has received a connection from (%s)\n", client->net.ip);
	//This socket descriptor is no longer needed.
	close(serv->net.sd);
	return client->net.sd;
}

//Receives an HTTP request.
void request(struct server_info *serv, struct client_info *client)
{
	int bytes;
	
	if ((bytes = recv(client->net.sd, client->net.buffer, KBYTE - 1, 0)) == -1)
		die("recv() error", client->net.sd, NULL);
	else
		client->net.buffer[bytes] = '\0';
	printf("Received (%i) bytes:\n%s", bytes, client->net.buffer);
	//Process the HTTP request and builds a response.
	handle_request(serv, client);
	//
	if (send(client->net.sd, serv->net.buffer, strlen(serv->net.buffer), 0) == -1)
		die("send() error", client->net.sd, NULL);
	//Close the socket descriptor.
	close(client->net.sd);
}

//Processes the HTTP request and builds a response.
void handle_request(struct server_info *serv, struct client_info *client)
{
	char date_time[30], url[PATH_MAX], response[KBYTE], *file_data, *token;
	int idx, len, i;
	long file_size;
	struct http_info http;
	
	idx = 0;
	http.code = OK;
	//Add the server's public HTML directory to the URL.
	snprintf(url, PATH_MAX, "%s", serv->path);
	//Parse the HTTP request line by line.
	for (token = strtok(client->net.buffer, "\n"); token != NULL; token = strtok(NULL, "\n")) {
		//Get the user's request.
		if (strstr(token, "GET") != NULL)
			sscanf(token, "%*s %"CAT(NAME_MAX)"s %*s", client->request);
		//Get the user agent.
		else if (strstr(token, "User-Agent") != NULL)
			sscanf(token, "%*s %300[^\n]s", client->agent);
	}
	//Append the client's request to the URL.
	strncat(url, client->request, PATH_MAX);
	//If the requested URL ends with a forward slash '/' then append the default directory index.
	if ((len = strlen(client->request)) == 1 || client->request[len - 1] == '/')
		strncat(url, serv->index, PATH_MAX);
	//Inspect the requested page for bad characters.
	for (i = 0; i < len; i++) {
		if (client->request[i] == '.' || client->request[i] == '$') {
			http.code = BAD_REQUEST;
			break;
		}
	}
	//Attempt to load the web page into memory.
	if (http.code == OK) {
		if ((file_size = load_file(url, &file_data)) == 0)
			http.code = NOT_FOUND;
	}
	//Set the HTTP status message.
	http_status(http.message, http.code);
	//Get the local time and date.
	get_local_time(date_time, 30);
	//Build the HTTP response line by line.
	idx = build_response(serv->net.buffer, idx, "HTTP/1.1 %i %s", http.code, http.message);
	idx = build_response(serv->net.buffer, idx, "Date: %s", date_time);
	idx = build_response(serv->net.buffer, idx, "Server: %s %s", serv->name, serv->version);
	idx = build_response(serv->net.buffer, idx, "Last-Modified: %s", date_time);
	idx = build_response(serv->net.buffer, idx, "ETag: %s", "\"56d-9989200-1132c580\"");
	idx = build_response(serv->net.buffer, idx, "Content-Type: %s", "text/html");
	idx = build_response(serv->net.buffer, idx, "Content-Length: %i", file_size);
	idx = build_response(serv->net.buffer, idx, "Accept-Ranges: %s", "bytes");
	idx = build_response(serv->net.buffer, idx, "Connection: %s", "close");
	strncat(serv->net.buffer, "\r\n", KBYTE);
	//Concat the web file.
	if (http.code == OK)
		strncat(serv->net.buffer, file_data, KBYTE);
	//Free up memory.
	FREE(file_data);
}

//Sets the HTTP response message.
void http_status(char *message, int code)
{
	switch (code) {
		case BAD_REQUEST:
			strncpy(message, "BAD REQUEST", 100);
			break;
		case FORBIDDEN:
			strncpy(message, "FORBIDDEN", 100);
			break;
		case NOT_FOUND:
			strncpy(message, "NOT FOUND", 100);
			break;
		case INTERNAL_ERROR:
			strncpy(message, "INTERNAL_ERROR", 100);
			break;
		case NOT_IMPLEMENTED:
			strncpy(message, "NOT IMPLEMENTED", 100);
			break;
		default:
			strncpy(message, "OK", 100);
	}
}

//Adds a new entry in the HTTP response.
int build_response(char *buffer, int idx, const char *format, ...)
{
	va_list args;
	
	if (idx == 0)
		memset(buffer, '\0', KBYTE);
	va_start(args, format);
	idx += vsnprintf(buffer + idx, KBYTE - idx, format, args);
	idx += snprintf(buffer + idx, KBYTE - idx, "\r\n");
	va_end(args);
	return idx;
}

//Retrieve and format the local time.
void get_local_time(char *ptime, size_t max)
{
	struct tm * time_info;
	time_t raw_time;
	
	memset(ptime, '\0', max);
	time(&raw_time);
	time_info = localtime(&raw_time);
	strftime(ptime, max, "%a, %d %h %Y %T GMT", time_info);
}

//Terminate the program and display debugging information.
void die(const char * s, int sock, struct addrinfo * res)
{
	#ifdef DEBUG_MODE
	perror(s);
	#endif
	if (sock)
		close(sock);
	if (res)
		freeaddrinfo(res);
	exit(EXIT_FAILURE);
}
