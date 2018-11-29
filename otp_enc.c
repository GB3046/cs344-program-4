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
	// checks if too few arguments were given
	if (argc < 4)
	{
		fprintf(stderr, "Not enough arguments\n");
		exit(1);
	}

	// opens files to read
	int fdPlainText = open(argv[1], O_RDONLY);
	int fdKey = open(argv[2], O_RDONLY);

	// checks if both files were opened properly
	if (fdPlainText == -1 || fdKey == -1)
	{
		fprintf(stderr, "Failed to open files\n");
		exit(1);
	}

	// variables to store data from files
	char plainText[80000];
	char key[80000];
	memset(plainText, '\0', sizeof(plainText));
	memset(key, '\0', sizeof(key));

	// read plain text
	if (read(fdPlainText, plainText, sizeof(plainText)) == -1)
	{
		fprintf(stderr, "Failed to read plain text\n");
		exit(1);
	}

	// read key text
	if (read(fdKey, key, sizeof(key)) == -1)
	{
		fprintf(stderr, "Failed to read plain text\n");
		exit(1);
	}

	// get rid of extra newlines at the end of strings
	plainText[strlen(plainText) - 1] = '\0';
	key[strlen(key) - 1] = '\0';

	// get length of strings and check if key is shorter than cipher
	int plainTextLength = strlen(plainText);
	int keyLength = strlen(key);
	if (plainTextLength > keyLength)
	{
		fprintf(stderr, "Key is shorter than plain text\n");
		exit(1);
	}

	// checks if plain text has only valid characters
	char c;
	int i;
	for (i = 0; i < plainTextLength; ++i)
	{
		c = plainText[i];
		if (isupper(c) || isspace(c))
		{
			// do nothing
		}
		else
		{
			fprintf(stderr, "Invalid characters in plain text\n");
			exit(1);
		}
	}

	// checks if key has only valid characters
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

	// creates signature for server to identify who client is
	char signature[] = "otp_enc";

	// variable to hold message to send
	char message[200000];
	memset(message, '\0', sizeof(message));

	// combines all strings into one string to send
	sprintf(message, "%s$%s$%s", plainText, key, signature);

	// client network stuff
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[80000];

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

	// send message to server
	charsWritten = 0;
	int charsSentThisPass = 0;
	int messageLength = strlen(message);
	char *messageSegment = message;
	do
	{
		charsSentThisPass = send(socketFD, &message, strlen(message), 0);
		if (charsSentThisPass < 0)
		{
			fprintf(stderr, "Error writing to socket\n");
			exit(1);
		}
		if (charsSentThisPass < messageLength)
		{
			fprintf(stderr, "Unable to send entire message\n");
		}
		charsWritten += charsSentThisPass;
		//messageSegment += charsSentThisPass;

	} while (charsWritten < messageLength);//strlen(message));

	// receive message from server
	memset(message, '\0', sizeof(message));
	do
	{
		// Get the message from the client and display it
		memset(buffer, '\0', sizeof(buffer));
		charsRead = recv(socketFD, buffer, sizeof(buffer), 0); // Read the client's message from the socket
		if (charsRead < 0) error("ERROR reading from socket");
		strcat(message, buffer);

	} while (message[strlen(message)-1] != '\n');
	
	// checks if response from from correct server
	if (message[0] == '\n')
	{
		fprintf(stderr, "Tried connecting to wrong server\n");
		exit(2);
	}
	else
	{
		printf("%s", message); // prints out cipher text
	}

	close(socketFD); // Close the socket
	return 0;
}