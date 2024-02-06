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

#include "networks.h"
#include "safeUtil.h"
#include "pdulib.h"
#include "pollLib.h"
#include "handleTable.h"
#include "flags.h"


#define MAX_MESSAGE 200
#define MAX_INPUT 1400
#define MAX_HANDLER 100

#define DEBUG_FLAG 1

void addNewSocket(int socket);
void serverControl(int mainServerSocket);
int checkArgs(int argc, char *argv[]);
void processNewClient(int clientSocket, uint8_t *dataBuffer);
void processClient(int clientSocket);
void processDirectMessage(int clientSocket, uint8_t *dataBuffer, int messageLen);
void processBroadcast(int clientSocket, uint8_t *dataBuffer, int messageLen);
void processList(int clientSocket);
void processExit(int clientSocket);
