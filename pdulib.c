#include "pdulib.h"

int sendPDU(int clientSocket, uint8_t *dataBuffer, int lengthOfData, int pduFlag)
{
    // PDU header is 3 bytes long (2 for length, 1 for flag)
    uint16_t pdu_len = lengthOfData + 3, pdu_len_net, bytesSent;
    uint8_t pdu_buffer[pdu_len];
    memset(pdu_buffer, 0, pdu_len);

    // Convert pdu_header_len to network byte order
    pdu_len_net = htons(pdu_len);

    // Copy pdu_len_net, pduFlag, and dataBuffer into pdu_buffer
    memcpy(pdu_buffer, &pdu_len_net, 2);
    pdu_buffer[2] = pduFlag;
    memcpy(pdu_buffer + 3, dataBuffer, lengthOfData);

    // Send pdu_buffer to clientSocket
    bytesSent = send(clientSocket, pdu_buffer, pdu_len, 0);
    if (bytesSent < 0)
    {
        perror("sendPDU");
        exit(EXIT_FAILURE);
    }
    // Return the number of bytes sent (excluding the 2 bytes for pdu_len and 1 byte for pduFlag)
    return bytesSent - 3;
}

int recvPDU(int clientSocket, uint8_t *dataBuffer, int bufferSize, int *pduFlag)
{
    uint16_t pdu_len_net, pdu_len;

    // Get PDU length from clientSocket
    uint16_t bytesReceived = recv(clientSocket, dataBuffer, 3, MSG_WAITALL);
    if (bytesReceived <= 0)
    {
        if (bytesReceived == 0)
        {
            // Connection closed
            return 0;
        } else {
            perror("recvPDU");
            exit(EXIT_FAILURE);
        }
    }

    // Convert PDU length from network to host byte order
    memcpy(&pdu_len_net, dataBuffer, 2);

    // Why 6? Shouldn't be 3 for the pdu_len and flag? It just works with minus 6
    pdu_len = ntohs(pdu_len_net) - 3; 

    // Get pduFlag from clientSocket
    *pduFlag = dataBuffer[2];

    // Check PDU length is not greater than bufferSize
    if (pdu_len > bufferSize)
    {
        fprintf(stderr, "recvPDU: Buffer size was too small\n");
        exit(EXIT_FAILURE);
    }

    // Get rest of the PDU from clientSocket
    bytesReceived = recv(clientSocket, dataBuffer, pdu_len, MSG_WAITALL);
    if (bytesReceived <= 0)
    {
        if (bytesReceived == 0)
        {
            return 0;
        } else {
            perror("recvPDU");
            exit(EXIT_FAILURE);
        }
    }
    // Return the number of bytes received (including the 3 bytes for the pdu_len and pduFlag)
    return bytesReceived;



}