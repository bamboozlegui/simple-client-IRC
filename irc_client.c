#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define BUFF_LENGTH 1024

// usage: irc_client [nick] [host] [port]

int main(int argc, char* argv[])
{
	char nick[16];
	const char allowed_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]\\`^{}-_|";
	char user[100];
	char host[100];
	unsigned int port;
	struct addrinfo hints;
	struct addrinfo *addr, *host_res;
	char buffer[BUFF_LENGTH] = { 0 };
	fd_set fds;
	int irc_sock;

	if (argc != 4)
	{
		fprintf(stderr, "Missing arguments: %s <nick> <host> <port>", argv[0]);
		exit(1);
	}

	strcpy(host, argv[2]);
	port = atoi(argv[3]);

	if (port < 1 || port > 65535)
	{
		printf("Port number out of range. Allowed: [1,65535]");
		exit(1);
	}
	else if (strlen(argv[1]) > 9 || strspn(argv[1], allowed_chars) != strlen(argv[1]))
	{
		fprintf(stderr, "Nickname is not allowed/too long.");
		exit(1);
	}

	sprintf(nick, "NICK %s\r\n", argv[1]);
	sprintf(user, "USER %s localhost %s\r\n", nick, nick);
	

	// hints is used to get addr info about address we're interested in
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Don't specify if IPv4 or IPv6 address, whichever gets hit
	hints.ai_socktype = SOCK_STREAM; // IRC uses TCP

	// check if socket creation and connection succeeds
	if (getaddrinfo(host, argv[3], &hints, &host_res) != 0)
	{
		fprintf(stderr, "Cannot find server.");
		exit(1);
	}

	// getaddrinfo returns list of address structs
	//next we try each address until we succesffully connect
	for (addr = host_res; addr != NULL; addr->ai_next)
	{					
		irc_sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		// ai_family hits either IPv6 or IPv4 addr, socktype is defined above
		if (irc_sock == -1)
			continue;
		if (connect(irc_sock, addr->ai_addr, addr->ai_addrlen) != -1)
			break;
	}

	// if no address succeeded
	if (addr == NULL)
	{
		fprintf(stderr, "Could not connect to server.");
		exit(1);
	}

	freeaddrinfo(host_res); //  no longer needed

	// send NICK and USER to IRC server
	send(irc_sock, nick, strlen(nick), 0);
	send(irc_sock, user, strlen(user), 0);

	for (;;)
	{
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(irc_sock	, &fds);

		if (select(irc_sock + 1, &fds, NULL, NULL, NULL) < 0)
		{
			fprintf(stderr, "Failed while selecting fd.\n");
			exit(1);
		}

		if (FD_ISSET(irc_sock, &fds))
		{
			memset(buffer, 0, BUFF_LENGTH);
			// if recv returns 0 then server disconnected, sent PING or response
			if(recv(irc_sock, buffer, BUFF_LENGTH, 0) == 0)
			{
				fprintf(stdin, "Server closed connection.\n");
				break;
			}
			else if(strncmp(buffer, "PING", 4) == 0)
			{
				send(irc_sock, "PONG\r\n", 4, 0);
			}
			else
			{
				fprintf(stdin, "%s", buffer);
			}
		}
		else if (FD_ISSET(0, &fds)) // check if event occured on stdin
		{
			memset(buffer, 0, BUFF_LENGTH);
			// read input from user
			fgets(buffer, BUFF_LENGTH, stdin);
			// append CR and NL, send input
			strcat(buffer, "\r\n");
			send(irc_sock, buffer, strlen(buffer), 0);
		}
	}

	close(irc_sock);

	return 0;
}