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

#define BUFF_LENGTH 2048

// usage: irc_client [nick] [host] [port]

char* find_ping(char* msg);

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
		fprintf(stderr, "Missing arguments: %s <nick> <host> <port>.\n", argv[0]);
		exit(1);
	}

	strcpy(host, argv[2]);
	port = atoi(argv[3]);

	if (port < 1 || port > 65535)
	{
		printf("Port number out of range. Allowed: [1,65535].\n");
		exit(1);
	} 

	/* check if nickname is not too long (9 chars as  per RFC) and consists only of allowed characters*/
	if (strlen(argv[1]) > 9 || strspn(argv[1], allowed_chars) != strlen(argv[1]))
	{
		fprintf(stderr, "Nickname is not allowed/too long.\n");
		exit(1);
	}

	// format nick and user messages to send to server
	int len_nick = sprintf(nick, "NICK %s\r\n", argv[1]);
	int len_user = sprintf(user, "USER %s * %s %s\r\n", argv[1], argv[2], argv[1]);
	
	
	// hints is used to get addr info about address we're interested in
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Don't specify if IPv4 or IPv6 address, whichever gets hit
	hints.ai_socktype = SOCK_STREAM; // IRC uses TCP

	// check if socket creation and connection succeeds
	if (getaddrinfo(host, argv[3], &hints, &host_res) != 0)
	{
		fprintf(stderr, "Cannot find server.\n");
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
		fprintf(stderr, "Could not connect to server.\n");
		exit(1);
	}

	freeaddrinfo(host_res); //  no longer needed

	// send NICK and USER to IRC server
	fprintf(stdout, "%s%s\n", nick, user);
	send(irc_sock, nick, len_nick, 0);
	send(irc_sock, user, len_user, 0);
	
	//char pong[BUFF_LENGTH];
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
			//memset(pong, 0, BUFF_LENGTH);
			// if recv returns 0 then server disconnected, sent PING or response
			if(recv(irc_sock, buffer, BUFF_LENGTH, 0) == 0)
			{
				fprintf(stdout, "Server closed connection.\n");
				break;
			}
			else
			{
				fprintf(stdout, "Processing server input...\n");
				char* line = strtok(buffer, "\r\n");
				while (line != NULL)
				{
					fprintf(stdout, "%s\r\n", line);
					//strcpy(pong, find_ping(line));
					char* pong = find_ping(line);
					if (pong != NULL)
					{
						fprintf(stdout, "%s\r\n", pong);
						send(irc_sock, pong, strlen(pong), 0);
						pong == NULL;
					}
					line = strtok(NULL, "\r\n");
				}
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

char* find_ping(char *msg)
{
	char *str, *result;
	int position;

	if (strstr(msg, "PING :") == msg)
	{
		//const char* ping_val = strchr(msg, ':');
		//snprintf(str, sizeof(msg), "PONG :%s\r\n", ping_val + 1);
		str = msg;
		str[1] = 'O';
	}
	else
		str = NULL;

	//fprintf(stdout, "%s", str);

	return str;
}