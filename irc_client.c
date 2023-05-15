#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFF_LENGTH 1024

// usage: irc_client [nick] [host] [port]

int main(int argc, char* argv[])
{
	char* nick;
	char* host;
	unsigned int port;
	struct addrinfo hints;
	struct addrinfo* addr, host_res;
	char buffer[BUFF_LENGTH] = { 0 };
	fd_set fds;
	int irc_sock;

	if (argc != 4)
	{
		fprintf(stderr, "Missing arguments: %s <nick> <host> <port>", argv[0]);
		exit(-1);
	}

	nick = argv[0];
	host = argv[1];
	port = atoi(argv[2]);

	if (port < 1 || port > 65535)
	{
		printf("Port number out of range. Allowed: [1,65535]");
		exit(1);
	}


	// hints is used to get addr info about address we're interested in
	memest(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Don't specify if IPv4 or IPv6 address, whichever gets hit
	hints.ai_socktype = SOCK_STREAM; // IRC uses TCP

	// check if socket creation and connection succeeds
	if (addrinfo(host, port, &hints, &host_res) == 0)
	{
		for (addr = host_res; addr != NULL; addr->ai_next)
		{					
			// ai_family hits either IPv6 or IPv4 addr, socktype is defined above, 
			if (irc_sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol) != -1)
				if (connect(irc_sock, addr->ai_addr, addr->ai_addrlen) != -1)
					break;
		}
	}
	else
		fprintf(stderr, "Cannot connect to server.");
	// send NICK and USER to IRC server
	send(irc_sock, "NICK &s", nick);
	send(irc_sock, "USER %s localhost %s %s");

	for (;;)
	{

		
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(fds, &fds);

		if (select(irc_sock + 1, &fds, NULL, NULL, NULL) < 0)
		{
			fprintf(stderr, "Failed while selecting fd.\n");
			exit(1);
		}

		if (FD_ISSET(irc_sock, &fds)
		{
			memset(buffer, 0, BUFF_LENGTH);
			// if recv returns 0 then server disconnected, sent PING or response
			if(recv(irc_sock, buffer, BUFF_LENGTH) == 0)
			{
				fprintf(stdin, "Server closed connection.\n");
				break;
			}
			else if(strncmp(buffer, "PING", 4))
			{
				send(irc_socket, "PONG", 4, 0);
			}
			else
			{
				fprintf(stdin, buffer);
			}
		}
		else if (FD_ISSET(0, &read_fds)) // check if event occured on stdin
		{
			memset(buffer, 0, BUFF_LENGTH);
			// read input from user
			fgets(buffer, BUFF_LENGTH, stdin);
			// send input
			send(client_socket, buffer, strlen(buffer), 0);
		}
	}

	close(irc_sock);

	return 0;
}