/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	server.c - A TCP server for the chat program using multiplex
--
--	PROGRAM:		server.exe
--
--	FUNCTIONS:		int main (int argc, char **argv)
--                  static void SystemFatal(const char* message)
--
--	DATE:			April 07, 2020
--
--
--	DESIGNERS:		Sam Lee
--
--	PROGRAMMERS:	Sam Lee
--
--	NOTES:
--	The program will accept connections from clients for chatting between clients.
--  It uses multiplexing I/O to monitor sockets waiting they become readable. If one of
--  them become readable, it reads the message on the socket and sends it to other clients.
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	1024		//Buffer length
#define CLIENTNUM 30
#define TRUE	1
#define LISTENQ	5

// Function Prototypes
static void SystemFatal(const char* );
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
--                 argv : user specified port
--
-- RETURNS: int
--
-- NOTES:
-- This function is a driver function that runs this application. This function is in charge on the main process which
-- accepts connections from clients and select readable socket to read messages and send them to other clients.
----------------------------------------------------------------------------------------------------------------------*/
int main (int argc, char **argv)
{
    int i, maxi, nready, bytes_to_read, arg;
    int listen_sd, new_sd, sockfd, client_len, port, maxfd, client[FD_SETSIZE];
    struct sockaddr_in server, client_addr;
    char rbuf[BUFLEN], sbuf[BUFLEN+CLIENTNUM], cNum[CLIENTNUM];
    ssize_t n;
    fd_set rset, allset;

    switch(argc)
    {
        case 1:
            port = SERVER_TCP_PORT;	// Use the default port
            break;
        case 2:
            port = atoi(argv[1]);	// Get user specified port
            break;
        default:
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            exit(1);
    }

    // Create a stream socket
    if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        SystemFatal("Cannot Create Socket!");

    // set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
    arg = 1;
    if (setsockopt (listen_sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
        SystemFatal("setsockopt");

    // Bind an address to the socket
    bzero(&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

    if (bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1)
        SystemFatal("bind error");

    // Listen for connections
    // queue up to LISTENQ connect requests
    listen(listen_sd, LISTENQ);

    maxfd	= listen_sd;	// initialize
    maxi	= -1;		// index into client[] array

    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;             // -1 indicates available entry
    FD_ZERO(&allset);
    FD_SET(listen_sd, &allset);


    while (TRUE)
    {
        rset = allset;               // structure assignment
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listen_sd, &rset)) // new client connection
        {
            client_len = sizeof(client_addr);
            if ((new_sd = accept(listen_sd, (struct sockaddr *) &client_addr, (socklen_t*)&client_len)) == -1)
                SystemFatal("accept error");

            printf(" Remote Address:  %s, port: %d\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));

            for (i = 0; i < FD_SETSIZE; i++)
                if (client[i] < 0)
                {
                    client[i] = new_sd;	// save descriptor
                    break;
                }
            if (i == FD_SETSIZE)
            {
                printf ("Too many clients\n");
                exit(1);
            }

            FD_SET (new_sd, &allset);     // add new descriptor to set
            if (new_sd > maxfd)
                maxfd = new_sd;	// for select

            if (i > maxi)
                maxi = i;	// new max index in client[] array

            if (--nready <= 0)
                continue;	// no more readable descriptors
        }

        for (i = 0; i <= maxi; i++)	// check all clients for data
        {
            if ((sockfd = client[i]) < 0)
                continue;

            if (FD_ISSET(sockfd, &rset))
            {
                bytes_to_read = BUFLEN;
                if ((n = read(sockfd, rbuf, bytes_to_read)) > 0)
                {
                    sprintf(cNum, "Client %d: ", i);
                    int j;
                    for(j = 0; j<=maxi; j++) {
                        if(i!=j){
                            sockfd = client[j];
                            strcpy(sbuf, cNum);
                            strcat(sbuf, rbuf);
                            write(sockfd, sbuf, BUFLEN+CLIENTNUM);
                        }
                    }
                    memset(rbuf, 0, sizeof(rbuf));
                    memset(sbuf, 0, sizeof(sbuf));
                    sockfd = client[i];
                }

                if (n == 0) // connection closed by client
                {
                    printf(" Remote Address:  %s closed connection\n", inet_ntoa(client_addr.sin_addr));
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                }

                if (--nready <= 0)
                    break;        // no more readable descriptors
            }
        }
    }
    return(0);
}
/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: SystemFatal
--
-- DATE: April 07, 2020
--
-- DESIGNER: Sam Lee
--
-- PROGRAMMER: Sam Lee
--
-- INTERFACE: static void SystemFatal(const char* message)
--                 message: error message
--
-- RETURNS: void
--
-- NOTES:
-- This function is designed to be called when an error occurs. It prints the error message and terminates the program.
----------------------------------------------------------------------------------------------------------------------*/
// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message)
{
    perror (message);
    exit (EXIT_FAILURE);
}