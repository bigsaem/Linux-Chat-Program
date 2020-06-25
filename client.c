/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	client.c - A TCP client for chat program
--
--	PROGRAM:		client.exe
--
--	FUNCTIONS:		int main (int argc, char **argv)
--                  void* ThreadFunc (void* sharedSt)
--
--	DATE:			April 07, 2020
--
--
--	DESIGNERS:		Sam Lee
--
--	PROGRAMMERS:	Sam Lee
--
--	NOTES:
--	The program will connect to the chat program server using TCP.
--  After establishing the connection, this app can send a message to the server which
--  delivers that message to other connected clients. This app also can receives messages
--  from other clients through the server and display it on the terminal.
---------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#define SERVER_TCP_PORT		7000	// Default port
#define BUFLEN              1024
#define RBUFLEN             1054// Buffer length
//shared socket structure
struct socketSt{
    int sd;
    char *bp[BUFLEN];
    int bytes_to_read;
    int running;
}socketSt;

void* ThreadFunc (void * clientStruct);

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: main
--
-- DATE: April 07, 2020
--
-- DESIGNER: Sam Lee
--
-- PROGRAMMER: Sam Lee
--
-- INTERFACE: main (int argc, char **argv)
--                 argc : args number
--                 argv : host name and user specified port
--
-- RETURNS: int
--
-- NOTES:
-- This function is a driver function that runs this application. This function is in charge on the main process which
-- connects to the server and sends messages. It creates a thread for reading part.
----------------------------------------------------------------------------------------------------------------------*/
int main (int argc, char **argv)
{
	int port;
	struct hostent	*hp;
	struct sockaddr_in server;
	char  *host, buf[BUFLEN],  **pptr;
	char str[16];
    pthread_t readingThread;
	switch(argc)
	{
		case 2:
			host =	argv[1];	// Host name
			port =	SERVER_TCP_PORT;
		break;
		case 3:
			host =	argv[1];
			port =	atoi(argv[2]);	// User specified port
		break;
		default:
			fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
			exit(1);
	}

	// Create the socket
	if ((socketSt.sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(1);
	}
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if ((hp = gethostbyname(host)) == NULL)
	{
		fprintf(stderr, "Unknown server address\n");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

	// Connecting to the server
	if (connect (socketSt.sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't connect to server\n");
		perror("connect");
		exit(1);
	}
	printf("Connected:    Server Name: %s\n", hp->h_name);
	pptr = hp->h_addr_list;
	printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));
	socketSt.running = 1;

    pthread_create (&readingThread, NULL, ThreadFunc, (void*)&socketSt);


	// Transmit data through the socket
	while(socketSt.running){
        fgets (buf, BUFLEN, stdin);
        fflush(stdout);
        send (socketSt.sd, buf, BUFLEN, 0);
	}
	close (socketSt.sd);
	return (0);
}
/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: ThreadFunc
--
-- DATE: April 07, 2020
--
-- DESIGNER: Sam Lee
--
-- PROGRAMMER: Sam Lee
--
-- INTERFACE: void* ThreadFunc (void* sharedSt)
--            void* sharedSt : this is a struct that contains the connected socket information and buffer
-- RETURNS: void*
--
-- NOTES:
-- This function is a function that the additionally created thread runs. This function is responsible for reading
-- messages form the server and display it to the terminal. It also senses the disconnection and changes the flag value
-- to terminate the program.
----------------------------------------------------------------------------------------------------------------------*/
void* ThreadFunc (void* sharedSt)
{
    struct socketSt* sst = (struct socketSt*)sharedSt;
    sst->bytes_to_read = RBUFLEN;
    int n;
	while ((n=recv (sst->sd, sst->bp, sst->bytes_to_read, 0)) > 0)
    {
        printf("%s", (char *)sst->bp);
	}
    printf("Connection closed. Please press any key and enter to close the application.");
	sst->running = 0;
    return NULL;
}
