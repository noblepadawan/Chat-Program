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
	int handleLen = dataBuffer[0];
	strncpy(handle, (char *)dataBuffer + 1, handleLen);
	handle[handleLen] = '\0';

	if (addHandle(&handleTable, clientSocket, handle) < 0)
	{
		// Handle already in use
		char errorMessage[MAX_INPUT] = {0};
		sprintf(errorMessage, "Handle already in use: %s", handle); 
		sendPDU(clientSocket, (uint8_t *)errorMessage, MAX_INPUT, CONNECT_ERR);
		
		// Close connection
		removeFromPollSet(clientSocket);
		close(clientSocket);

	} else {
		// Handle added
		sendPDU(clientSocket, NULL, 0, CONNECT_ACK);
	}
}

void processBroadcast(int clientSocket, uint8_t *dataBuffer, int messageLen)
{
	// Send message to all clients except the sender
	for (int i = 0; i < handleTable.size; i++)
	{
		if (handleTable.entries[i].socket != clientSocket)
		{
			sendPDU(handleTable.entries[i].socket, dataBuffer, messageLen, BROADCAST);
		}
	}
}

void processDirectMessage(int clientSocket, uint8_t *dataBuffer, int messageLen)
{
	// Send message to specific client or multiple clients
	// Format: senderHandleLength + senderHandle + numOfDestinations + destinationHandleLength + destinationHandle + message

	char destHandle[MAX_HANDLER];
	int destHandleLen;
	int offset = 2 + dataBuffer[0];

	destHandleLen = dataBuffer[offset];
	offset++;
	memcpy(destHandle, dataBuffer + offset, destHandleLen);
	destHandle[destHandleLen] = '\0';

	// Find the socket associtated with the destination handle
	int destSocket = getSocketByHandle(&handleTable, destHandle);
	if (destSocket == -1)
	{
		// Handle not found
		char error[MAX_INPUT] = {0};
		error[0] = strlen(destHandle);	// Length of handle
		sprintf(error + 1, "Client with handle %s does not exist", destHandle);	// Error message
		sendPDU(clientSocket, (uint8_t *)error, MAX_INPUT, ERROR);
	} else {
		// Handle found
		sendPDU(destSocket, dataBuffer, messageLen, MESSAGE);
	}
}

void processList(int clientSocket)
{
	// Get the number of handles
	uint32_t numHandles = handleTable.size;
	uint32_t numHandlesNet = htonl(numHandles);

	// Send the number of handles
	sendPDU(clientSocket, (uint8_t *)&numHandlesNet, 4, LIST_RESP);

	// Send each individual handle
	for (int i = 0; i < handleTable.size; i++)
	{
		uint8_t handleLen = strlen(handleTable.entries[i].handle);
		// Calculate the total size of the PDU: handle length + handle
		int pduSize = 1 + handleLen;

		// Create the buffer to hold the PDU
		uint8_t pduBuffer[pduSize];
		pduBuffer[0] = handleLen; // First byte is the length of the handle
		strncpy((char *)pduBuffer + 1, handleTable.entries[i].handle, handleLen);

		// Send the handle PDU
		sendPDU(clientSocket, pduBuffer, pduSize, LIST_HANDLE);
	}

	// Send the end of list PDU
	sendPDU(clientSocket, NULL, 0, LIST_END);

}

void processExit(int clientSocket)
{
	// Send exit ack
	sendPDU(clientSocket, NULL, 0, EXIT_ACK);
	
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

	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAX_INPUT, &pduFlag)) < 0)
	{
		perror("recv call");
		exit(-1);
	}
	if (messageLen > 0)
	{
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

