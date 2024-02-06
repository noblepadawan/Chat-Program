/******************************************************************************
* cclient.c
*
* Writen by Kieran Valino, updated Jan 2024
* Modified by Kieran Valino
*
*****************************************************************************/

#include "cclient.h"

// Globals
char clientHandle[MAX_INPUT];
uint8_t clientLen;

int main(int argc, char * argv[])
{
	int socketNum = setup(argc, argv);
	clientControl(socketNum);

	close(socketNum);
	
	return 0;
}

int setup(int argc, char * argv[])
{	
	checkArgs(argc, argv);

	strncpy(clientHandle, argv[1], MAX_INPUT);
	clientLen = strlen(clientHandle);
	
	if (strlen(clientHandle) > MAX_HANDLER || strlen(clientHandle) <= 0)
	{
		fprintf(stderr, "Invalid handle, handle longer than 100 characters: %s\n", clientHandle);
		exit(-1);
	}
	if (!isalpha(clientHandle[0]))
	{
		fprintf(stderr, "Invalid handle, handle starts with a number\n");
		exit(-1);
	}
	for (int i = 0; i < strlen(clientHandle); i++)
	{
		if(!isalnum(clientHandle[i]))
		{
			fprintf(stderr, "Invalid handle, handle must only contain alphanumeric characters: %s\n", clientHandle);
			exit(-1);
		}
	}

	/* set up the TCP Client socket  */
	int socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	// Send connection request to server
	uint8_t sendBuf[MAX_INPUT];
	int sendLen = buildConnectPDU(sendBuf);
	sendPDU(socketNum, sendBuf, sendLen, CONNECT);

	// Wait for connection response from server
	serverConnectResponse(socketNum);

	return socketNum;
}

void clientControl(int clientSocket)
{
	setupPollSet();
	addToPollSet(clientSocket);
	addToPollSet(STDIN_FILENO);

	int fd;
	while (1)
	{
		printf("$: ");
		fflush(stdout);
		fd = pollCall(-1);
		if (fd == STDIN_FILENO)
		{
			processInput(clientSocket);
		}
		else if (fd == clientSocket)
		{
			processMessageFromServer(clientSocket);
		}
	}
}


int buildConnectPDU(uint8_t * sendBuf)
{
	// Build the connect PDU
	memset(sendBuf, 0, MAX_INPUT);
	sendBuf[0] = clientLen;
	memcpy(sendBuf + 1, clientHandle, clientLen);
	return clientLen + 1;
}

void serverConnectResponse(int socketNum)
{
	char recvBuf[MAX_INPUT];
	int receiveLen = 0;
	int pduFlag;
	if ((receiveLen = recvPDU(socketNum, (uint8_t *)recvBuf, MAX_INPUT, &pduFlag)) < 0)
	{
		perror("recv call");
		exit(-1);
	}
	if (pduFlag == CONNECT_ACK)
	{
		return;
	}
	else if (pduFlag == CONNECT_ERR)
	{
		printf("%s\n", recvBuf);
		exit(-1);
	} else {
		printf("Server has terminated\n");
		exit(-1);
	}
}

int checkValidInput(uint8_t* input)
{

	// Check if there is no space after the command
	if (input[2] != ' ' && input[2] != '\0')
	{
		fprintf(stderr, "Invalid command format\n");
		return 1;
	}

	if (input[0] != '%')
	{
		fprintf(stderr, "Invalid command format\n");
		return 1;
	}

	return 0;

}

void processMessageFromServer(int clientSocket)
{
	char recvBuf[MAX_INPUT] = {0};
	int receiveLen = 0;
	int pduFlag = 0;
	if ((receiveLen = recvPDU(clientSocket, (uint8_t *) recvBuf, MAX_INPUT, &pduFlag)) < 0)
	{
		perror("recv call");
		exit(-1);
	}
	if (receiveLen >= 0)
	{
		switch (pduFlag)
		{
			case MESSAGE:
				// Handle message
				handleMessage((uint8_t *)recvBuf, receiveLen);
				break;
			case MULTICAST:
				// Handle multicast
				handleMessage((uint8_t *)recvBuf, receiveLen);
				break;
			case LIST_RESP:
				// Handle list response
				handleList((uint8_t *)recvBuf, receiveLen, clientSocket);
				break;
			case BROADCAST:
				// Handle broadcast
				handleBroadcast((uint8_t *)recvBuf, receiveLen);
				break;
			case ERROR:
				// Handle error
				printf("\n%s\n", recvBuf + 1);
				break;	
			case EXIT_ACK:
				// Handle exit ack
				printf("\nExiting\n");
				removeFromPollSet(clientSocket);
				close(clientSocket);
				exit(0);
				break;
			default:
				printf("\nServer has terminated\n");
				exit(0);
		}
	}
}

void handleList(uint8_t * recvBuf, int receiveLen, int clientSocket)
{
	// Get total number handles from server
	int numHandles;
	memcpy(&numHandles, recvBuf, 4);
	numHandles = ntohl(numHandles);
	printf("\nNumber of clients: %d\n", numHandles);

	char handlebuf[MAX_HANDLER] = {0};
	int pduFlag = 0;

	while(pduFlag != LIST_END)
	{
		if ((receiveLen = recvPDU(clientSocket, (uint8_t *) handlebuf, MAX_INPUT, &pduFlag)) < 0)
		{
			perror("recv call");
			exit(-1);
		}
		// receiveLen = receiveLen - 3;
		if (receiveLen >= 0)
		{
			if (pduFlag == LIST_END)
			{
				return;
			}
			if (pduFlag == LIST_HANDLE)
			{
				int handleLen = handlebuf[0];
				char handle[MAX_HANDLER];
				memcpy(handle, handlebuf + 1, handleLen);
				handle[handleLen] = '\0';
				printf("\t%s\n", handle);
			}
		}
	}
	printf("Server has terminated\n");
	exit(-1);

}

void handleBroadcast(uint8_t * recvBuf, int receiveLen)
{
	// Format: senderHandleLength + senderHandle + message
	int senderHandleLen = recvBuf[0];
	char senderHandle[MAX_HANDLER] = {0};
	char message[MAX_MESSAGE] = {0};
	int offset = senderHandleLen + 1;

	// Get the sender handle
	memcpy(senderHandle, recvBuf + 1, senderHandleLen);

	// Get the message
	memcpy(message, recvBuf + offset, receiveLen - offset);

	printf("\n%s: %s\n", senderHandle, message);
}

void handleMessage(uint8_t * recvBuf, int receiveLen)
{
	// Format: senderHandleLength + senderHandle + numOfDestinations + destinationHandleLength + destinationHandle + message
	int senderHandleLen = recvBuf[0];
	char senderHandle[MAX_HANDLER] = {0};
	char message[MAX_MESSAGE] = {0};
	int offset = senderHandleLen + 2;

	// Get the sender handle
	memcpy(senderHandle, recvBuf + 1, senderHandleLen);

	// Move the offset to the start of the message
	offset += recvBuf[offset] + 1;

	// Get the message
	memcpy(message, senderHandle, senderHandleLen);
	message[senderHandleLen] = ':';
	message[senderHandleLen + 1] = ' ';
	memcpy(message + senderHandleLen + 2, recvBuf + offset, receiveLen - offset);

	printf("\n%s\n", message);
}

int splitMessage(char * userMessage, char messages[MAX_PACKETS][MAX_MESSAGE])
{
	// Split the message into packets of 200 characters and store them in messages
	// Return the number of packets
	int numPackets = 0;
	int messageLen = strlen(userMessage);

	// Ensure there is at least one character in the message
    if (messageLen == 0) {
        return 0;
    }

	// Calculate the number of packets needed
    numPackets = (messageLen + MAX_MESSAGE - 1) / MAX_MESSAGE;

    // Split the message into packets
    for (int i = 0; i < numPackets; i++) {
        strncpy(messages[i], userMessage + i * MAX_MESSAGE, MAX_MESSAGE - 1);
        messages[i][MAX_MESSAGE - 1] = '\0';  // Null-terminate each packet
    }
	return numPackets;
}

void sendMessage(int clientSocket, uint8_t * input, int sendLen)
{
	uint8_t sendBuf[MAX_INPUT] = {0};
	char userMessage[MAX_INPUT], messages[MAX_PACKETS][MAX_MESSAGE];
	int handleLen;
	int numPackets;
	int i, j;
	char destHandle[MAX_HANDLER];

	// Get the destination handle
    for (i = 3; i < MAX_INPUT && input[i] != ' ' && input[i] != '\0'; i++) {
        destHandle[i - 3] = input[i];
    }
    destHandle[i - 3] = '\0';
	handleLen = strlen(destHandle);

	// Get the message
    for (j = i + 1; (j < MAX_INPUT) && (input[j] != '\0'); j++) {
        userMessage[j - i - 1] = input[j];
    }
    userMessage[j - i - 1] = '\0';

	// Split the message into packets
	numPackets = splitMessage(userMessage, messages);

	// Iterate through the packets and send each one
	for (int i = 0; i < numPackets; i++) {
    	// Format the data to send as senderHandleLength + senderHandle + numOfDestinations + destinationHandleLength + destinationHandle + message
    	sendBuf[0] = clientLen;                                             			// Sender handle length
    	memcpy(sendBuf + 1, clientHandle, clientLen);                       			// Sender handle
    	sendBuf[clientLen + 1] = 1;                                          			// Number of destinations
    	sendBuf[clientLen + 2] = handleLen;                                  			// Destination handle length
    	memcpy(sendBuf + clientLen + 3, destHandle, handleLen);             			// Destination handle
    	strncpy((char *)(sendBuf + clientLen + 3 + handleLen), messages[i], MAX_MESSAGE - 1);  	// Message packet

    	// Send the message packet to the server
    	sendPDU(clientSocket, sendBuf, clientLen + 3 + handleLen + strlen(messages[i]), MESSAGE);
	}

}

void sendMulticast(int clientSocket, uint8_t * input, int sendLen)
{
	// Send multicast to server
	uint8_t sendBuf[MAX_INPUT] = {0};
	char userMessage[MAX_INPUT] = {0}, messages[MAX_PACKETS][MAX_MESSAGE] = {0};
	int handleLen;
	int numPackets;
	int numOfHandles;
	int handleCount;
	int i = 5, j = 0, k = 0;
	char destHandles[9][MAX_HANDLER] = {0};

	// Get the number of destinations
	numOfHandles = input[3] - '0';

	// Check if the number of destinations is valid
	if( numOfHandles < 2 || numOfHandles > 9)
	{
		// Invalid number of destinations
		fprintf(stderr, "Invalid number of destinations\n");
		return;
	}

	// Get all destination handles
	for (handleCount = 0; handleCount < numOfHandles; handleCount++) {
		for (j = i; (j < MAX_INPUT) && (input[j] != ' ') && (input[j] != '\0'); j++) {
			destHandles[handleCount][j - i] = input[j];
		}
		destHandles[handleCount][j - i] = '\0';
		i = j + 1;
	}

	// Get the message
	for (k = i; (k < MAX_INPUT) && (input[k] != '\0'); k++) {
		userMessage[k - i] = input[k];
	}

	// Split the message into packets
	numPackets = splitMessage(userMessage, messages);

	// Iterate through the destination handles and send the message to each one
	for (handleCount = 0; handleCount < numOfHandles; handleCount++) {
		for (int packetIndex = 0; packetIndex < numPackets; packetIndex++) {
			// Format the data to send as senderHandleLength + senderHandle + numOfDestinations + destinationHandleLength + destinationHandle + message
			sendBuf[0] = clientLen;                                         									// Sender handle length
			memcpy(sendBuf + 1, clientHandle, clientLen);                   									// Sender handle
			sendBuf[clientLen + 1] = numOfHandles;                            									// Number of destinations
			handleLen = strlen(destHandles[handleCount]);
			sendBuf[clientLen + 2] = handleLen;                             									// Destination handle length
			memcpy(sendBuf + clientLen + 3, destHandles[handleCount], handleLen);         						// Destination handle
			strncpy((char *)(sendBuf + clientLen + 3 + handleLen), messages[packetIndex], MAX_MESSAGE - 1);  	// Message packet

			// Send the message packet to the server
			sendPDU(clientSocket, sendBuf, clientLen + 3 + handleLen + strlen(messages[packetIndex]), MULTICAST);
		}
	}
}

void sendList(int clientSocket)
{
	// Send list request to server
	char sendBuf[MAX_INPUT] = {0};
	sendBuf[0] = clientLen;
	memcpy(sendBuf + 1, clientHandle, clientLen);
	sendPDU(clientSocket, (uint8_t *)sendBuf, clientLen + 1, LIST_REQ);
}

void sendBroadcast(int clientSocket, uint8_t * input, int sendLen)
{
	// Send broadcast to server
	uint8_t sendBuf[MAX_INPUT] = {0};

	sendBuf[0] = clientLen;
	memcpy(sendBuf + 1, clientHandle, clientLen);
	strncpy((char *)(sendBuf + clientLen + 1), (char *)(input + 3), sendLen - 3);
	sendPDU(clientSocket, sendBuf, clientLen + sendLen - 3, BROADCAST);

}

void processInput(int clientSocket)
{
	uint8_t inputbuf[MAX_INPUT];
	int inputLen;
	uint8_t cmd;

	// Get the user input
	inputLen = readFromStdin(inputbuf);


	// Chack if input is valid
	if (checkValidInput(inputbuf))
	{
		// Invalid input
		return;
	}

	cmd = tolower(inputbuf[1]);
	switch (cmd)
	{
		case 'm':
			// Send message to handle
			sendMessage(clientSocket, inputbuf, inputLen);
			break;
		case 'b':
			// Broadcast to server
			sendBroadcast(clientSocket, inputbuf, inputLen);
			break;
		case 'c':
			// Multicast to server
			sendMulticast(clientSocket, inputbuf, inputLen);
			break;
		case 'l':
			// List all client handles
			sendList(clientSocket);
			break;
		case 'e':
			// Exit
			sendPDU(clientSocket, NULL, 0, EXIT);
			break;
		default:
			// Invalid command
			fprintf(stderr, "Invalid command\n");

	}

}

int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	while (inputLen < (MAX_INPUT - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle-name host-name port-number \n", argv[0]);
		exit(1);
	}
}
