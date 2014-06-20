/*
	Copyright © 2014 Shipping Soon. All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#define HOST "127.0.0.1"
#define PORT "6301"
#define MAXDATASIZE 1000

void die(const char * s, int sock, struct addrinfo * res)
{
	perror(s);
	if (sock)
		close(sock);
	if (res)
		freeaddrinfo(res);
	exit(1);
}

int main(void)
{
	struct addrinfo hints, * res;
	int sd, bytes, error;
	char buf[MAXDATASIZE];
	char * say = "123456789\n123456789\n123456789\n123456789\n---------\n";
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if ((error = getaddrinfo(HOST, PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
		return 2;
	}
	
	if((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
		die("An error has occurred", 0, res);
	
	if (connect(sd, res->ai_addr, res->ai_addrlen) == -1)
		die("An error has occurred", sd, res);
		
	struct sockaddr_in * tmp = (struct sockaddr_in *) res->ai_addr;
	printf("Client has received connection from (%s)\n", inet_ntoa(tmp->sin_addr));
	
	freeaddrinfo(res);
	
	printf("Sending (%i) bytes\n", strlen(say));
	if (send(sd, say, strlen(say), 0) == -1)
		die("An error has occurred", sd, NULL);
		
	if ((bytes = recv(sd, buf, MAXDATASIZE-1, 0)) == -1)
		die("An error occurred", sd, NULL);
	else
		buf[bytes] = '\0';
	printf("Received (%i) bytes:\n%s", bytes, buf);
	
	close(sd);
	printf("\n-----------------------------\n");
	return 0;
}
