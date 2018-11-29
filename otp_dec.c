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

void error(const char *msg) { perror(msg); exit(2); } // Error function used for reporting issues

int main(int argc, char const *argv[])
{

	if (argc < 4)
	{
		fprintf(stderr, "Not enough arguments\n");
		exit(1);
	}

	int fdCipherText = open(argv[1], O_RDONLY);
	int fdKey = open(argv[2], O_RDONLY);

	if (fdCipherText == -1 || fdKey == -1)
	{
		fprintf(stderr, "Failed to open files\n");
		exit(1);
	}

	char cipherText[80000];
	char key[80000];
	memset(cipherText, '\0', sizeof(cipherText));
	memset(key, '\0', sizeof(key));

	if (read(fdCipherText, cipherText, sizeof(cipherText)) == -1)
	{
		fprintf(stderr, "Failed to read plain text\n");
		exit(1);
	}

	if (read(fdKey, key, sizeof(key)) == -1)
	{
		fprintf(stderr, "Failed to read plain text\n");
		exit(1);
	}
	cipherText[strlen(cipherText) - 1] = '\0';
	key[strlen(key) - 1] = '\0';

	int cipherTextLength = strlen(cipherText);
	int keyLength = strlen(key);
	if (cipherTextLength > keyLength)
	{
		fprintf(stderr, "Key is shorter than plain text\n");
		exit(1);
	}

	char c;
	int i;
	for (i = 0; i < cipherTextLength; ++i)
	{
		c = cipherText[i];
		if (isupper(c) || isspace(c))
		{
			// do nothing
		}
		else
		{
			fprintf(stderr, "Invalid characters in cipher text\n");
			exit(1);
		}
	}

	//int i;
	for (i = 0; i < keyLength; ++i)
	{
		c = key[i];
		if (isupper(c) || isspace(c))
		{
			// do nothing
		}
		else
		{
			fprintf(stderr, "Invalid characters in key\n");
			exit(1);
		}
	}

	char signature[] = "otp_dec";

	char message[200000];
	memset(message, '\0', sizeof(message));

	sprintf(message, "%s$%s$%s", cipherText, key, signature);


	// client network stuff
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[80000];
    
	//if (argc < 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	// Get input message from user
	// printf("CLIENT: Enter text to send to the server, and then hit enter: ");
	// memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	// fgets(buffer, sizeof(buffer) - 1, stdin); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	// buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds

	// Send message to server
	// charsWritten = send(socketFD, message, strlen(message), 0); // Write to the server
	// if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	// if (charsWritten < strlen(message)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	charsWritten = 0;
	int charsSentThisPass = 0;
	do
	{
		charsSentThisPass = send(socketFD, &message, strlen(message), 0);
		if (charsSentThisPass < 0)
		{
			fprintf(stderr, "Error writing to socket\n");
			exit(1);
		}
		charsWritten += charsSentThisPass;

	} while (charsWritten < strlen(message));

	// Get return message from server
	// memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	// charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	// if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	memset(message, '\0', sizeof(message));
	do
	{
		// Get the message from the client and display it
		memset(buffer, '\0', sizeof(buffer));
		charsRead = recv(socketFD, buffer, sizeof(buffer), 0); // Read the client's message from the socket
		if (charsRead < 0) error("ERROR reading from socket");
		strcat(message, buffer);

	} while (message[strlen(message)-1] != '\n');
	
	//printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
	if (message[0] == '\n')
	{
		fprintf(stderr, "Tried connecting to wrong server\n");
	}
	else
	{
		printf("%s", message);
	}

	close(socketFD); // Close the socket
	return 0;
}