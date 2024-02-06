#ifndef HANDLE_TABLE_H
#define HANDLE_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HANDLES 100

typedef struct {
    int socket;             // Socket associated with the handle
    char handle[100];       // Handle name
} HandleEntry;

typedef struct {
    HandleEntry *entries;   // Array of handle entries
    int size;               // Current number of handles in the table
    int capacity;           // Total capacity of the handle table
} HandleTable;

void initializeHandleTable(HandleTable *handle_table);
int getSocketByHandle(const HandleTable *handle_table, const char *handle);
const char *getHandleBySocket(const HandleTable *handle_table, int socket);
int addHandle(HandleTable *handle_table, int socket, const char *handle);
void removeHandle(HandleTable *handle_table, int socket);

#endif
