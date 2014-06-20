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
#include "client.h"


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
	struct sockaddr_in * tmp;
	int sd, bytes, error;
	char buf[KBYTE];
	char msg[KBYTE];
	
	strncpy(msg, "GET / HTTP/1.1\r\nHost: localhost:6301\r\nUser-Agent: cClient\r\n", KBYTE);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((error = getaddrinfo(HOST, PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(error));
		return 2;
	}
	if((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
		die("socket() error", 0, res);
	if (connect(sd, res->ai_addr, res->ai_addrlen) == -1)
		die("connect() error", sd, res);
	tmp = (struct sockaddr_in *) res->ai_addr;
	printf("Client has received connection from (%s)\n", inet_ntoa(tmp->sin_addr));
	freeaddrinfo(res);
	if (send(sd, msg, strlen(msg), 0) == -1)
		die("send() error", sd, NULL);
	if ((bytes = recv(sd, buf, KBYTE-1, 0)) == -1)
		die("recv() error", sd, NULL);
	else
		buf[bytes] = '\0';
	printf("Received (%i) bytes:\n%s", bytes, buf);
	close(sd);
	return 0;
}
