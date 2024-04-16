/******************************************************************************
 * HTTP Server
 * CPE 3300, Daniel Nimsgern
 *
 * Build with gcc -o tftpclient tftpclient.c
 *****************************************************************************/

/*=============================================================================
 |          Includes
 ============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

/*=============================================================================
 |          Global Variables
 ============================================================================*/

/* Network definitions */
#define DEFAULT_HTTP_PORT   (int)   80
#define HTTP_MAX_MSG_SIZE   (int)   50000

/* CLI ESC Codes */
#define ESC_BLACK_TXT       (char*) "\033[1;30m"
#define ESC_RED_TXT         (char*) "\033[1;31m"
#define ESC_GREEN_TXT       (char*) "\033[1;32m"
#define ESC_YELLOW_TXT      (char*) "\033[1;33m"
#define ESC_BLUE_TXT        (char*) "\033[1;34m"
#define ESC_MAGENTA_TXT     (char*) "\033[1;35m"
#define ESC_CYAN_TXT        (char*) "\033[1;36m"
#define ESC_WHITE_TXT       (char*) "\033[1;37m"
#define ESC_BR_GRAY_TXT     (char*) "\033[1;90m"
#define ESC_BR_RED_TXT      (char*) "\033[1;91m"
#define ESC_BR_GREEN_TXT    (char*) "\033[1;92m"
#define ESC_BR_YELLOW_TXT   (char*) "\033[1;93m"
#define ESC_BR_BLUE_TXT     (char*) "\033[1;94m"
#define ESC_BR_MAGENTA_TXT  (char*) "\033[1;95m"
#define ESC_BR_CYAN_TXT     (char*) "\033[1;96m"
#define ESC_BR_WHITE_TXT    (char*) "\033[1;97m"

/*=============================================================================
 |          Function Definitions
 ============================================================================*/

/**
 * @brief Main process - Handles HTTP requests to the server from clients
 * 
 * @param argc Number of server configuration arguments
 * @param argv Array of server configuration arguments
 * @return int Program exit value
 */
int main(int argc, char** argv) {

	// Local Variables
	unsigned short port = DEFAULT_HTTP_PORT; // default port
	int sock; // socket descriptor

    // User argument parsing
    int c;
    while((c = getopt(argc,argv,"p:h"))!=-1)
    {
		switch(c) 
		{
			case 'p':
				port = atof(optarg);
				break;
			case 'h':
				printf("\n");
                printf("-h prints this help statement\n\n");
                printf("-p override the HTTP server port (default: 80)\n\n");
		        exit(1);
				break;
		}
	}

	// ready to go
	printf("HTTP server over TCP configuring on port: %d\n",port);
	
	// for TCP, we want IP protocol domain (PF_INET)
	// and TCP transport type (SOCK_STREAM)
	// no alternate protocol - 0, since we have already specified IP
	
	if ((sock = socket( PF_INET, SOCK_STREAM, 0 )) < 0) 
	{
		perror("Error on socket creation");
		exit(1);
	}
  
  	// lose the pesky "Address already in use" error message
	int yes = 1;

	if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) 
	{
		perror("setsockopt");
		exit(1);
	}

	// establish address - this is the server and will
	// only be listening on the specified port
	struct sockaddr_in sock_address;
	
	// address family is AF_INET
	// our IP address is INADDR_ANY (any of our IP addresses)
    // the port number is per default or option above

	sock_address.sin_family = AF_INET;
	sock_address.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_address.sin_port = htons(port);

	// we must now bind the socket descriptor to the address info
	if (bind(sock, (struct sockaddr *) &sock_address, sizeof(sock_address))<0)
	{
		perror("Problem binding");
		exit(-1);
	}
	
	// extra step to TCP - listen on the port for a connection
	// willing to queue 5 connection requests
	if ( listen(sock, 5) < 0 ) 
	{
		perror("Error calling listen()");
		exit(-1);
	}
	
	// go into forever loop and echo whatever message is received
	// to console and back to source
	char* buffer = (HTTP_MAX_MSG_SIZE, sizeof(char));
	struct sockaddr_in callingDevice;
	socklen_t callingDevice_len;
    int fileLen;
	int bytesRead;
    int bytesWritten;
	int connection;
	
    while (1) {			
		// hang in accept and wait for connection
		printf("====Waiting====\n");
		connection = accept(sock, (struct sockaddr*)&callingDevice,
                            &callingDevice_len);
		if (connection < 0)
		{
			perror("ERROR on accept");
		}

        // Fork on connection to allow for multiple client connection
		int pid = fork();
		if (pid < 0)
		{
			perror("ERROR on fork");
		}

        // Child Process
		if (pid == 0)
		{		
            close(sock);
			// ready to r/w - another loop - it will be broken when the
            // connection is closed
			while(1)
			{
				// read message								
				bytesRead = read(connection, buffer, HTTP_MAX_MSG_SIZE-1);
						
				if (bytesRead == 0)
				{	// socket closed
					printf("====Client Disconnected====\n");
					close(connection);
					break;  // break the inner while loop
				}
						
				// see if client wants us to disconnect
				if (strncmp(buffer,"quit",4)==0)
				{
					printf("====Server Disconnecting====\n");
					close(connection);
					break;  // break the inner while loop
				}
			
				// print info to console
				printf("Received HTTP request\n");

				// send data to HTTP client
				if ( (bytesWritten = write(connection, buffer, fileLen)) < 0 )
				{
					perror("Error sending echo");
					exit(-1);
				}
				else
				{			
					printf("Bytes echoed: %d\n", bytesWritten);
				}
				
			}  // end of child inner-while
			close(sock);
			exit(0);
		}
        else
        {
            close(connection);
        }
    }	// end of outer loop
	free(buffer);
	// should never get here
	return(0);
}
