# Chat Program

A chat program where clients can chat with other clients by using a TCP connection to connect to the main server which receives custom application-level PDUs from the clients and fowards them to its respective destination.

## Server Installation
```
make server
```

## Server Usage
```
./server [port-number]
```

## Client Installation
```
make cclient
```

## Client Usage
```
./cclient <handle-name> <host-name> <port-number>
```

## Client Commands
1. Send a message to a different client on the server
```
$: %M <destination-handle> [message]
```

2. Send a multicast message to multiple clients on the server
```
$: %C <numnber-of-handles> <destination-handle> <destination-handle> [destination-handle][message]
```

3. Send a broadcast message to all clients on the server
```
$: %B [message]
```

4. List all the clients currently connected to the server
```
$: %L
```

5. Disconnect from the server
```
$: %E
```
