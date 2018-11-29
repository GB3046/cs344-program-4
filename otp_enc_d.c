#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

int main(int argc, char const *argv[])
{
	// variables for server initialization
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[200000];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// loop forever until killed
	while(1)
	{
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		// forks after new connection is made
		int pid = fork();
		int exitStatus;

		// variables to hold strings for incoming and outgoing messages
		char message[200000];
		char plainText[80000];
		char key[80000];
		char signature[2400];
		char cipherText[80000];
		memset(message, '\0', sizeof(message));
		memset(plainText, '\0', sizeof(plainText));
		memset(key, '\0', sizeof(key));
		memset(signature, '\0', sizeof(signature));
		memset(cipherText, '\0', sizeof(cipherText));

		// variables used to encode message
		int plainTextLength = 0;
		int plainNum;
		int keyNum;
		int cipherNum;
		int i;

		switch (pid) // switch based on if parent or child
		{
			case -1: // if fork failed
				fprintf(stderr, "Forking error\n");
				exit(1);
			case 0: // child process
				// get message from client		
				do
				{
					memset(buffer, '\0', sizeof(buffer));
					charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer), 0); // Read the client's message from the socket
					if (charsRead < 0) error("ERROR reading from socket");
					strcat(message, buffer);
				} while (message[strlen(message)-1] != 'c');

				// separate message into usable pieces
				sscanf(message, "%[^'$']$%[^'$']$%s", plainText, key, signature);

				// checks if correct client is talking to server
				if (strcmp(signature, "otp_enc") != 0)
				{
					//fprintf(stderr, "Wrong server\n");
					send(establishedConnectionFD, "\n", 2, 0);
					exit(1);
				}

				// find length of plain text
				plainTextLength = strlen(plainText);

				// loop through plain text and encode characters one by one
				for (i = 0; i < plainTextLength; ++i)
				{
					plainNum = plainText[i]; // convert char to int

					if (plainNum == 32) // if char is space
					{
						plainNum = 0;
					}
					else // else char is uppercase letter
					{
						plainNum -= 64;
					}

					keyNum = key[i]; // convert char to int
					if (keyNum == 32) // if char is space
					{
						keyNum = 0;
					}
					else // else char is uppercase letter
					{
						keyNum -= 64;
					}

					// do encoding math
					cipherNum = (plainNum + keyNum) % 27;

					if (cipherNum == 0) // char is space
					{
						cipherText[i] = ' ';
					}
					else // else char is uppercase letter
					{
						cipherText[i] = cipherNum + 64; // store encoded char
					}
				}

				// add newline to end of string
				cipherText[strlen(cipherText)] = '\n';

				// Send encoded message to client
				charsRead = send(establishedConnectionFD, &cipherText, strlen(cipherText), 0); // Send success back
				if (charsRead < 0) error("ERROR writing to socket");
				if (charsRead < strlen(cipherText))
				{
					fprintf(stderr, "Unable to send entire message\n");
				}

				exit(0); // kill child process
			default: // parent
				close(establishedConnectionFD); // Close the existing socket which is connected to the client			
				// wait for child to die
				do
				{
					waitpid(pid, &exitStatus, 0);
				} while (!WIFEXITED(exitStatus) && !WIFSIGNALED(exitStatus));
		}
	}
	close(listenSocketFD); // Close the listening socket
	return 0;
}