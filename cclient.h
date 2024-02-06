#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>

#include "networks.h"
#include "safeUtil.h"
#include "pdulib.h"
#include "pollLib.h"
#include "flags.h"

#define MAX_MESSAGE 200
#define MAX_INPUT 1400
#define MAX_HANDLER 100
#define MAX_PACKETS 7

#define DEBUG_FLAG 1

int setup(int argc, char * argv[]);
void clientControl(int clientSocket);
int buildConnectPDU(uint8_t * sendBuf);
void serverConnectResponse(int socketNum);
int checkValidInput(uint8_t* input);
void sendList(int clientSocket);
void processInput(int clientSocket);
void processMessageFromServer(int clientSocket);
void handleMessage(uint8_t * recvBuf, int receiveLen);
void sendMessage(int clientSocket, uint8_t * input, int inputLen);
int splitMessage(char * userMessage, char messages[MAX_PACKETS][MAX_MESSAGE]);
void sendMulticast(int clientSocket, uint8_t * input, int sendLen);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void handleMessage(uint8_t * recvBuf, int receiveLen);
void handleList(uint8_t * recvBuf, int receiveLen, int clientSocket);
void handleBroadcast(uint8_t * recvBuf, int receiveLen);