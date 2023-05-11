#include <arpa/inet.h>
#include <argp.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "../include/util.h"

//Tahyr Bayryyev

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

// Stores the command line arguments 
// Can store other arguments in struct
typedef struct args {
    struct sockaddr_in server_sockaddr;
} client_arguments;

// Simply store the values from the command line arguments into the struct
// Checking the values are correct are done in client_parseopt
error_t client_parser(int key, char *arg, struct argp_state *state) {
	client_arguments *args = state->input;
	error_t ret = 0;

	switch(key) {
	case 'a': // Handles IP Address
		args->server_sockaddr.sin_family = AF_INET; // AF_INET == IPv4 address family
        
        // Convert string to binary representation
		if (inet_pton(AF_INET, arg, &args->server_sockaddr.sin_addr) == 0) {
			argp_error(state, "Invalid address");
		}
		break;
	case 'p': // Handles Ports
        args->server_sockaddr.sin_port = htons(atoi(arg)); // Converts to network byte order
		break;
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}

// Calls parser to store the arguments into the client_arguments struct
// Checks that the inputs are valid
int client_parseopt(int argc, char *argv[], client_arguments *ptr) {
	struct argp_option options[] = {
		{ "addr", 'a', "addr", 0, "The IP address the server is listening at", 0},
		{ "port", 'p', "port", 0, "The port that is being used at the server", 0},
        {0}
	};

	struct argp argp_settings = { options, client_parser, 0, 0};
	bzero(ptr, sizeof(*ptr)); // Zeroes out client arguments

    // Parses the arguments
	if (argp_parse(&argp_settings, argc, argv, 0, NULL, ptr) != 0) {
		perror("Got error in client_parseopt\n");
        return 1;
	}
    
	/* 
       Checks that the values inputted are both present and correct
	*/

 	if (ptr->server_sockaddr.sin_addr.s_addr == 0) {
        perror("No server_sockaddr address input\n");
        return EXIT_FAILURE;
    }  else if (ptr->server_sockaddr.sin_family == 0) {
        perror("No server_sockaddr sin_family input\n");
        return EXIT_FAILURE;
    } else if (ptr->server_sockaddr.sin_port == 0) {
        perror("No server_sockaddr sin_port input\n");
        return EXIT_FAILURE;
    } else if (ntohs(ptr->server_sockaddr.sin_port <= 0)) {
		perror("No port input found\n");
		return EXIT_FAILURE;
	} else if (ntohs(ptr->server_sockaddr.sin_port) < 1024 || ntohs(ptr->server_sockaddr.sin_port) > 65535) {
		perror("Error: Port must be in range [1024, 65535].\n");
		return EXIT_FAILURE;
	} 

    return EXIT_SUCCESS;
}


int handle_stdin() { 
    // Get the string from standard input and store it in outbuf 
    int get_s = get_string(outbuf,MAX_MSG_LEN);

    // if the string is too long print to stdout not stderr
    if(get_s == TOO_LONG){
        printf("Sorry, limit your message to %d characters.\n" , MAX_MSG_LEN);
        return EXIT_FAILURE;
    }

    // send the message to the server and see if it fails to send
    if(send(client_socket, outbuf, strlen(outbuf), 0) == -1){
        fprintf(stderr, "Error: Failed to send message to server. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // if the user inputs bye then we terminate the program and close the client socket
    if (strcmp(outbuf,"bye") == 0){
        printf("Goodbye.\n");
        // close the client socket before exiting the program
        close(client_socket);
        exit(EXIT_SUCCESS);
        
    }    
    return EXIT_SUCCESS;
}

int handle_client_socket() { 
    // Receive data from the socket and store it in inbuf
    int bytes_recvd = recv(client_socket, inbuf, BUFLEN+1, 0);

    // if the number of bytes received is -1 
    if (bytes_recvd  == -1) {
        fprintf(stderr, "Warning: Failed to receive incoming message. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }
    
    //If the number of bytes received is 0, the server abruptly broke the connection with the client, 
    // as in the server crashed or perhaps the network failed. Error out 
    if(bytes_recvd == 0){
        fprintf(stderr, "\nConnection to server has been lost.\n");
        // close the client socket before exiting the program
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    
    // null terminate the string 
    inbuf[bytes_recvd] = '\0';
    
    // check if server inputed bye -> then we exit program otherwise print the message from server
    if(strcmp("bye", inbuf) == 0){
        printf("\nServer initiated shutdown.\n");
        // close the client socket before exiting the program
        close(client_socket);
        exit(EXIT_SUCCESS);
    
    } else{
        // add a new line to put the senders message on a new line in the receivers terminal
        printf("\n%s\n",inbuf);
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) { 

    // Parses client arguments
    client_arguments args_ptr;
    if(client_parseopt(argc, argv, &args_ptr) == EXIT_FAILURE) {
        perror("client_parseopt error\n");
        return EXIT_FAILURE;
    }
    
    // Prints out the console the server address and port for debugging. DELETE or COMMENT IT OUT
    char str[INET_ADDRSTRLEN]; // Stores IP Addr in human readable form
    // Prints out received info
    printf("Server Address: %s on port: %u\n",
        inet_ntop(AF_INET, &(args_ptr.server_sockaddr.sin_addr), str, INET_ADDRSTRLEN), 
        ntohs(args_ptr.server_sockaddr.sin_port)
    );
    fflush(stdout);


    // get the username from the user. Keeps asking user for username until valid input
    int get_username = NO_INPUT;
    while(get_username != OK){
        printf("Enter your username: ");
        fflush(stdout);
        // get the username using the get_string function in util.h
        get_username = get_string(username,MAX_NAME_LEN+1);

        // if there is no input then continue to get the username
        if(get_username == NO_INPUT){
            continue;
        }

        // if the username is too long print an error message and continue to ask the user for their username
        if(get_username == TOO_LONG){
            printf("Sorry, limit your username to %d characters.\n" , MAX_NAME_LEN);
            continue;
        }
    }

    // message printed after obtaining the username successfully
    printf("Hello, %s. Let's try to connect to the server.\n", username);

    /* =========================================
              connecting to the server
       ========================================= */ 

    // create the TCP socket AF_INET [IPv4 address family] same domain as sin_family
    // set the client_socket fd to the fd returned from creating the socket
    // Uses TCP since SOCK_STREAM
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // check if socket failed to create
    if (client_socket == -1) {
        fprintf(stderr, "Error: Failed to create socket. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // connect to server and see if connection to the server fails (-1 on error,  0 if succeeds)
    if(connect(client_socket,(struct sockaddr *)&args_ptr.server_sockaddr, sizeof(args_ptr.server_sockaddr)) == -1){
        fprintf(stderr, "Error: Failed to connect to server. %s.\n", strerror(errno));
        close(client_socket); // close the client socket before exiting the program
        return EXIT_FAILURE;
    }

    // try to receive the welcome message from the server. 
    int recvbytes = recv(client_socket, inbuf, BUFLEN+1, 0);

    // check if messaged failed to be recieved from the server
    if (recvbytes == -1) {
        fprintf(stderr, "Error: Failed to receive message from server. %s.\n", strerror(errno));
        close(client_socket); // close the client socket before exiting the program
        return EXIT_FAILURE;
    }

    // if no bytes are received then that means that too many people are on the server
    if(recvbytes == 0){
        fprintf(stderr, "All connections are busy. Try again later.\n");
         // close the client socket before exiting the program
        close(client_socket);
        return EXIT_FAILURE;
    }

    // If the connection is successful, print the following: a new line, the welcome message received
    // from the server, and two more new lines.
    printf("\n");
    printf("%s", inbuf);
    printf("\n");
    printf("\n");

    // Finally, send the username to the server. If it fails, error out 
    if(send(client_socket, username, strlen(username), 0) == -1){
        fprintf(stderr, "Error: Failed to send username to server. %s.\n", strerror(errno));
        // close the client socket before exiting the program
        close(client_socket);
        return EXIT_FAILURE;
    }

    // your program should now loop forever, prompting the user
    // for input and determining if there is activity on one of the two file descriptors

    // initialize and fd set called active_fd_set
    fd_set active_fd_set;

    while(true){

        // print username each time 
        printf("[%s]: ", username);
        fflush(stdout);


        /* Zero out active fd_set */
        FD_ZERO(&active_fd_set);

        //both the client_socket and STDIN FILENO should be added to the fd set
        FD_SET(STDIN_FILENO, &active_fd_set);
        FD_SET(client_socket, &active_fd_set);

        /* 
            Use select() to actively monitor fds 
        */

        // check if select errors out
        if(select(client_socket+1, &active_fd_set, NULL, NULL, NULL)<0){
            fprintf(stderr, "Error: Select Failed.\n");
            // close the client socket before exiting the program
            close(client_socket);
            return EXIT_FAILURE;
        }
        
        // if STDIN_FILENO fd is a member of the set pointed to by active_fd_set then call handle_stdin()
        if(FD_ISSET(STDIN_FILENO, &active_fd_set)){
            handle_stdin();   
        }

        // if client_socket fd is a member of the set pointed to by active_fd_set then call handle_client_scoket()
        if(FD_ISSET(client_socket, &active_fd_set)){
            handle_client_socket();
              
        }
    }

    // close the client socket before exiting the program
    close(client_socket);
    return EXIT_SUCCESS;
 }
