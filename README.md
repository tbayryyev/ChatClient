# ChatClient
This chat client works with the sockets API in the C programming language. The chat client is able to communicate with a server by sending messages to the server which is then able to broadcast the message to all the other clients on the system. Similarly, the client is able to receive messages directly from the server, thus enabling the user to read messages sent by all other clients on the system.

### How to Run the ChatClient
Make sure you are within the project directory, then run the following: 
./chatclient -a {server ip address} -p {port numnber}
```
make chatclient
./chatclient -a 192.0.0.1 -p 3000
```
Note: order of arguments do not matter, port number must be in [1024, 65535]
