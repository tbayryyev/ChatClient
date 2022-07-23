#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "util.h"

// I pledge my honor that I have abided by the Stevens Honor System.
// Tahyr Bayryyev

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

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


// check for usage make sure the number of arguments are correct
    if(argc != 3){
        fprintf(stderr,"Usage: %s <server IP> <port>\n",argv[0]);
        return EXIT_FAILURE;

    }


// declare an object of sockaddr in and set it up first.
    struct sockaddr_in sa;

//To make sure all data in the memory space the object occupies are zeros, we need to use memset to clear it up
    memset(&sa, 0, sizeof(sa));


// check if the ip address is valid using inet_pton
    int ip = inet_pton(AF_INET, argv[1], &sa.sin_addr);
    if(ip<=0){
        fprintf(stderr,"Error: Invalid IP address '%s'.\n",argv[1]);
        return EXIT_FAILURE;
    }

    int port_number;
   
// check if the port number is a valid integer using parse_int
    if(parse_int(argv[2], &port_number, "port number") == false){
        return EXIT_FAILURE;
    }

// check the range of the port number and see if it is in the range
    if(port_number < 1024 || port_number > 65535){
        fprintf(stderr,"Error: Port must be in range [1024, 65535].\n");
        return EXIT_FAILURE;

    }


    // get the username from the user
    while(true){
        printf("Enter your username: ");
        fflush(stdout);
        // get the username using the get_string function in util.h
        int get_username = get_string(username,MAX_NAME_LEN+1);

        // if there is no input then continue to get the username
        if(get_username == NO_INPUT){
            continue;
        }

        // if the username is too long print an error message and continue to ask the user for their username
        if(get_username == TOO_LONG){
            printf("Sorry, limit your username to %d characters.\n" , MAX_NAME_LEN);
            continue;
        }

        // break from the loop once you get the username 
        break;
    }



// message printed after obtaining the username successfully
    printf("Hello, %s. Let's try to connect to the server.\n", username);

// connecting to the server


// initialize the values in the object sa

//sin_family indicates the domain use AF_INET
    sa.sin_family = AF_INET;
// initialize the port_number which we error checked earlier in the program
    sa.sin_port = htons(port_number);

    // create the TCP socket AF_INET same domain as sin_family (most commonly used domain)
    // set the client_socket fd to the fd returned from creating the socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // check if socket failed to create
    if (client_socket == -1) {
        fprintf(stderr, "Error: Failed to create socket. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // connect to server and see if connection to the server fails (-1 on error,  0 if succeeds)
    if(connect(client_socket,(struct sockaddr *) &sa, sizeof(sa)) == -1){
        fprintf(stderr, "Error: Failed to connect to server. %s.\n", strerror(errno));
        // close the client socket before exiting the program
        close(client_socket);
        return EXIT_FAILURE;
    }

//try to receive the welcome message from the server. 
    int recvbytes = recv(client_socket, inbuf, BUFLEN+1, 0);

    // check if messaged failed to be recieved from the server
    if (recvbytes == -1) {
        fprintf(stderr, "Error: Failed to receive message from server. %s.\n", strerror(errno));
        // close the client socket before exiting the program
        close(client_socket);
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


        /* Use select() to actively monitor fds */

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