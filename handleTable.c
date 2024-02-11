#include "handleTable.h"

void initializeHandleTable(HandleTable *handle_table) 
{
    // Initialize the handle table with a capacity of MAX_HANDLES
    handle_table->entries = malloc(MAX_HANDLES * sizeof(HandleEntry));
    handle_table->size = 0;
    handle_table->capacity = MAX_HANDLES;
}

int getSocketByHandle(const HandleTable *handle_table, const char *handle) 
{
    int i;
    for (i = 0; i < handle_table->size; i++) 
    {
        // Compare the handle with the current entry
        if (strcmp(handle_table->entries[i].handle, handle) == 0) 
        {
            return handle_table->entries[i].socket;
        }
    }
    return -1; // Handle not found
}

const char *getHandleBySocket(const HandleTable *handle_table, int socket) 
{
    int i;
    for (i = 0; i < handle_table->size; i++) 
    {
        // Compare the socket with the current entry
        if (handle_table->entries[i].socket == socket) 
        {
            return handle_table->entries[i].handle;
        }
    }
    return NULL; // Socket not found
}

int addHandle(HandleTable *handle_table, int socket, const char *handle) 
{
    // Resize the array if it's full by doubling its capacity
    if (handle_table->size == handle_table->capacity) 
    {
        handle_table->capacity *= 2;
        handle_table->entries = realloc(handle_table->entries, handle_table->capacity * sizeof(HandleEntry));
    }

    // Check if handle already exists
    if (getSocketByHandle(handle_table, handle) != -1) {
        return -1; // Handle already in use
    }

    // Add the handle entry to the handle table
    handle_table->entries[handle_table->size].socket = socket;
    // Copy the handle string into the handle entry
    strncpy(handle_table->entries[handle_table->size].handle, handle, sizeof(handle_table->entries[handle_table->size].handle));
    // Increment the size of the handle table
    handle_table->size++;

    return 0; // Success
}

void removeHandle(HandleTable *handle_table, int socket) 
{
    int i;
    for (i = 0; i < handle_table->size; i++) 
    {
        if (handle_table->entries[i].socket == socket) 
        {
            // Remove the handle entry by swapping with the last entry and decrementing the size
            handle_table->entries[i] = handle_table->entries[handle_table->size - 1];
            handle_table->size--;
            return;
        }
    }
}
