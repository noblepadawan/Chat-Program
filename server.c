/******************************************************************************
* server.c
* 
* Writen by Prof. Smith, updated Jan 2023
* Modified by Kieran Valino
*
*****************************************************************************/

#include "server.h"

// Globals
HandleTable handleTable;


int main(int argc, char *argv[])
{
	int mainServerSocket = 0;   //socket descriptor for the server socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	//initialize the handle table
	initializeHandleTable(&handleTable);

	//server control loop
	serverControl(mainServerSocket);

	//close the main server socket
	close(mainServerSocket);

	return 0;
}

void addNewSocket(int socket)
{
	int clientSocket = tcpAccept(socket, DEBUG_FLAG);
	addToPollSet(clientSocket);
}

void processNewClient(int clientSocket, uint8_t *dataBuffer)
{
	// Add new client to handle table
	char handle[MAX_HANDLER] = {0};
	int handleLen = dataBuffer[1];
	strncpy(handle, (char *)dataBuffer + 2, handleLen);
	handle[handleLen] = '\0';

	if (addHandle(&handleTable, clientSocket, handle) < 0)
	{
		// Handle already in use
		char errorMessage[MAX_INPUT] = {0};
		errorMessage[0] = CONNECT_ERR;
		sprintf(errorMessage + 1, "Handle already in use: %s", handle);
		sendPDU(clientSocket, (uint8_t *)errorMessage, MAX_INPUT);
		
		// Close connection
		removeFromPollSet(clientSocket);
		close(clientSocket);

	} else {
		// Handle added
		char acceptMessage[MAX_INPUT] = {0};
		acceptMessage[0] = CONNECT_ACK;
		sendPDU(clientSocket, (uint8_t *)acceptMessage, 1);
	}
}

void processBroadcast(int clientSocket, uint8_t *dataBuffer, int messageLen)
{
	// Send message to all clients except the sender
	int i;
	for (i = 0; i < handleTable.size; i++)
	{
		if (handleTable.entries[i].socket != clientSocket)
		{
			sendPDU(handleTable.entries[i].socket, dataBuffer, messageLen);
		}
	}
}

void processDirectMessage(int clientSocket, uint8_t *dataBuffer, int messageLen)
{
	// Send message to specific client or multiple clients
	// Format: senderHandleLength + senderHandle + numOfDestinations + destinationHandleLength + destinationHandle + message

	char destHandle[MAX_HANDLER];
	int destHandleLen;
	int offset = 3 + dataBuffer[1];

	destHandleLen = dataBuffer[offset];
	offset++;
	memcpy(destHandle, dataBuffer + offset, destHandleLen);
	destHandle[destHandleLen] = '\0';

	// Find the socket associtated with the destination handle
	int destSocket = getSocketByHandle(&handleTable, destHandle);
	if (destSocket == -1)
	{
		// Handle not found
		char errorMessage[MAX_INPUT] = {0};
		errorMessage[0] = ERROR;
		sprintf(errorMessage + 1, "Client with handle %s does not exist", destHandle);	// Error message
		sendPDU(clientSocket, (uint8_t *)errorMessage, MAX_INPUT);
	} else {
		// Handle found
		sendPDU(destSocket, dataBuffer, messageLen);
	}
}

void processList(int clientSocket)
{
	// Get the number of handles
	uint32_t numHandles = handleTable.size;
	uint32_t numHandlesNet = htonl(numHandles);
	uint8_t sendBuffer[MAX_INPUT] = {0};
	int i;

	// Copy the number of handles to the send buffer
	sendBuffer[0] = LIST_RESP;
	memcpy(sendBuffer + 1, &numHandlesNet, 4);

	// Send the number of handles
	sendPDU(clientSocket, sendBuffer, MAX_INPUT);

	// Send each individual handle
	for (i = 0; i < handleTable.size; i++)
	{
		uint8_t handleLen = strlen(handleTable.entries[i].handle);
		// Calculate the total size of the PDU: flag + handle length + handle
		int pduSize = 2 + handleLen;

		// Create the buffer to hold the PDU
		uint8_t pduBuffer[pduSize];
		pduBuffer[0] = LIST_HANDLE;
		pduBuffer[1] = handleLen; // First byte is the length of the handle
		memcpy(pduBuffer + 2, handleTable.entries[i].handle, handleLen); // Copy the handle to the buffer
		
		// Send the handle PDU
		sendPDU(clientSocket, pduBuffer, pduSize);
	}

	// Send the end of list PDU
	memset(sendBuffer, 0, MAX_INPUT);
	sendBuffer[0] = LIST_END;
	sendPDU(clientSocket, sendBuffer, 4);

}

void processExit(int clientSocket)
{
	char sendBuffer[1] = {EXIT_ACK};
	// Send exit ack
	sendPDU(clientSocket, (uint8_t *)sendBuffer, 1);
	
	// Remove client from handle table
	removeHandle(&handleTable, clientSocket);
	removeFromPollSet(clientSocket);
	close(clientSocket);
}

void processClient(int clientSocket)
{
	uint8_t dataBuffer[MAX_INPUT];
	int messageLen = 0;
	int pduFlag;

	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAX_INPUT)) < 0)
	{
		perror("recv call");
		exit(-1);
	}
	if (messageLen > 0)
	{
		pduFlag = dataBuffer[0];
		switch (pduFlag)
		{
			case CONNECT:
				// Process new client
				processNewClient(clientSocket, dataBuffer);
				break;
			case BROADCAST:
				// Broadcast message
				processBroadcast(clientSocket, dataBuffer, messageLen);
				break;
			case MESSAGE:
				// Direct message
				processDirectMessage(clientSocket, dataBuffer, messageLen);
				break;
			case MULTICAST:
				// Multicast message
				processDirectMessage(clientSocket, dataBuffer, messageLen);
				break;
			case EXIT: 
				// Close connection
				processExit(clientSocket);
				break;
			case LIST_REQ: 
				// List clients
				processList(clientSocket);
				break;
			default:
				printf("Unknown message on socket %d, length %d, with flag %d\n", clientSocket, messageLen, pduFlag);
		}
	} else {
		processExit(clientSocket);
	}
}

void serverControl(int mainServerSocket)
{
	int socket;
	setupPollSet();
	addToPollSet(mainServerSocket);
	while(1)
	{
		socket = pollCall(-1);
		if (socket == mainServerSocket)
		{
			// Accept new client
			addNewSocket(socket);
		} else {
			// Process client
			processClient(socket);
		}
	}
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}

