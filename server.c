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
#define PORT "6651"
#define BACKLOG 10
#define MAXDATASIZE 1000

void die(const char * s, int sock, struct addrinfo * res)
{
	perror(s);
	if (sock)
		close(sock);
	if (res)
		freeaddrinfo(res);
	exit(EXIT_FAILURE);
}

int main(void)
{
	struct addrinfo hints, * res;
	struct sockaddr_in dest;
	int sd, nsd, error, bytes, yes = 1, socksize = sizeof(struct sockaddr_in);
	char buf[MAXDATASIZE], s[INET_ADDRSTRLEN];
	char * reply = "Your message has been sent";
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	printf("Getting address info for (%s:%s)\n", HOST, PORT);
	if ((error = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
		return 1;
	}
	
	printf("Creating socket descriptor\n");
	if((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
		die("Socket error", 0, res);
	printf("Created socket (%i)\n", sd);
	
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		die("An error has occurred", sd, res);

	printf("Binding socket (%i) to port (%s)\n", sd, PORT);
	if ((bind(sd, res->ai_addr, res->ai_addrlen)) == -1)
		die("Bind error", sd, res);
	
	freeaddrinfo(res);
	
	if ((listen(sd, BACKLOG)) == -1) 
		die("Listen error", sd, NULL);
	printf("Socket (%i) is listening on port (%s)\n", sd, PORT);
	
	if ((nsd = accept(sd, (struct sockaddr *) &dest, &socksize)) == -1)
		die("An error has occurred", sd, NULL);
	printf("Server has received connection from (%s)\n", inet_ntoa(dest.sin_addr));
	
	close(sd);
	
	if ((bytes = recv(nsd, buf, MAXDATASIZE-1, 0)) == -1)
		die("An error occurred", nsd, NULL);
	else
		buf[bytes] = '\0';
	printf("Received (%i) bytes:\n%s", bytes, buf);
	
	if (send(nsd, reply, strlen(reply), 0) == -1)
		die("An error has occurred", nsd, NULL);
	
	close(nsd);
	printf("\n-----------------------------\n");
	return EXIT_SUCCESS;
}
