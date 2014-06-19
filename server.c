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
	int sd;
	
	//Set default values for our server's settings.
	config_init(&serv);
	//Load the configuration file into memory.
	if ((file_size = load_file(CONFIG_FILE_PATH, &file_data)) != 0)
		//Parse the configuration file line by line.
		parse_config(&serv, file_data, file_size, "\n");
	//Free the data.
	FREE(file_data);
	
	server(&serv, &client);
	response(&serv, &client);
	return EXIT_SUCCESS;
}

//Loads the contents of a file into memory. Note: does not free up memory.
long load_file(const char *file_path, char **buffer)
{
	FILE *file;
	long file_size;
	size_t result;
	
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
			sscanf(token, "%*s %s", serv->name);
		//Get the port number.
		else if (strstr(token, "Port") != NULL)
			sscanf(token, "%*s %s", serv->port);
		//Get the public HTML directory.
		else if (strstr(token, "Directory") != NULL)
			sscanf(token, "%*s %s", serv->path);
		//Get the server's version.
		else if (strstr(token, "Version") != NULL)
			sscanf(token, "%*s %s", serv->version);
	}
}

//Initiates the server.
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
		if ((serv->sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1)
			continue;
		break;
	}
	//Allow our socket to forcibly bind to a port.
	if (setsockopt(serv->sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		die("setsockopt() error", serv->sd, res);
	//Bind the socket to a port.
	if ((bind(serv->sd, res->ai_addr, res->ai_addrlen)) == -1)
		die("bind() error", serv->sd, res);
	//This dynamically allocated addrinfo is no longer needed.
	freeaddrinfo(res);
	//Make our socket descriptor listen on a port.
	if ((listen(serv->sd, BACKLOG)) == -1)
		die("listen() error", serv->sd, NULL);
	//Accept an incoming connection.
	if ((client->sd = accept(serv->sd, (struct sockaddr *) &dest, &sock_size)) == -1)
		die("accept() error", serv->sd, NULL);
	else
		strncpy(client->ip, inet_ntoa(dest.sin_addr), INET_ADDRSTRLEN);
	printf("Server has received a connection from (%s)\n", client->ip);
	//This socket descriptor is no longer needed.
	close(serv->sd);
	return client->sd;
}



//Builds the HTTP response.
void response(struct server_info *serv, struct client_info *client)
{
	char buffer[KBYTE], date_time[30], *file_data;
	int bytes, idx;
	long file_size;

	idx = 0;
	get_local_time(date_time, 30);
	idx = build_response(buffer, idx, "HTTP/1.1 %i %s", OK, "OK");
	idx = build_response(buffer, idx, "Date: %s", date_time);
	idx = build_response(buffer, idx, "Server: %s %s", serv->name, serv->version);
	idx = build_response(buffer, idx, "Last-Modified: %s", date_time);
	idx = build_response(buffer, idx, "ETag: %s", "\"56d-9989200-1132c580\"");
	idx = build_response(buffer, idx, "Content-Type: %s", "text/html");
	idx = build_response(buffer, idx, "Content-Length: %i", 110);
	idx = build_response(buffer, idx, "Accept-Ranges: %s", "bytes");
	idx = build_response(buffer, idx, "Connection: %s", "close");
	
	if ((bytes = recv(client->sd, client->request, KBYTE - 1, 0)) == -1)
		die("recv() error", client->sd, NULL);
	else
		client->request[bytes] = '\0';
	printf("Received (%i) bytes:\n%s", bytes, client->request);

	const char *resp = 
		"HTTP/1.1 200 OK\r\n"
		"Date: Thu, 19 Feb 2009 12:27:04 GMT\r\n"
		"Server: Apache/2.2.3\r\n"
		"Last-Modified: Wed, 18 Jun 2003 16:05:58 GMT\r\n"
		"ETag: \"56d-9989200-1132c580\"\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 110\r\n"
		"Accept-Ranges: bytes\r\n"
		"Connection: close\r\n"
		"\r\n"
		"<html>"
		"<head>"
		"<title>An Example Page</title>"
		"</head>"
		"<body>"
		"Hello World, this is a very simple HTML document."
		"</body>"
		"</html>";
	if (send(client->sd, resp, strlen(resp), 0) == -1)
		die("send() error", client->sd, NULL);
	//Close the socket descriptor.
	close(client->sd);
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
