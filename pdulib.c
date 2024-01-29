#include "pdulib.h"

int sendPDU(int clientSocket, uint8_t *dataBuffer, int lengthOfData)
{
    // PDU header is 2 bytes long
    uint16_t pdu_len = lengthOfData + 2, pdu_len_net, bytesSent;
    uint8_t pdu_buffer[pdu_len];

    // Convert pdu_header_len to network byte order
    pdu_len_net = htons(pdu_len);

    // Copy pdu_len_net and dataBuffer into pdu_buffer
    memcpy(pdu_buffer, &pdu_len_net, 2);
    memcpy(pdu_buffer + 2, dataBuffer, lengthOfData);

    // Send pdu_buffer to clientSocket
    bytesSent = send(clientSocket, pdu_buffer, lengthOfData + 2, 0);
    if (bytesSent < 0)
    {
        perror("sendPDU");
        exit(EXIT_FAILURE);
    }
    // Return the number of bytes sent (excluding the 2 bytes for the pdu_len)
    return bytesSent - 2;
}

int recvPDU(int clientSocket, uint8_t *dataBuffer, int bufferSize)
{
    uint16_t pdu_len_net, pdu_len;

    // Get PDU length from clientSocket
    uint16_t bytesReceived = recv(clientSocket, dataBuffer, 2, MSG_WAITALL);
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

    // Convert PDU length from network to host byte order
    memcpy(&pdu_len_net, dataBuffer, 2);
    pdu_len = ntohs(pdu_len_net);

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
    // Return the number of bytes received (excluding the 2 bytes for the pdu_len)
    return bytesReceived;



}