#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int IdOrigin = -1;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server IP> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int clientSocket;
    struct sockaddr_storage serverAddr;
    socklen_t addrSize;
    char buffer[BUFFER_SIZE] = "REQ_ADD";

    // Create socket
    if (strstr(argv[1], ":") != NULL) {
        // IPv6 address format
        clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
        ((struct sockaddr_in6 *)&serverAddr)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&serverAddr)->sin6_port = htons(atoi(argv[2]));
        if (inet_pton(AF_INET6, argv[1], &((struct sockaddr_in6 *)&serverAddr)->sin6_addr) <= 0) {
            perror("Invalid address");
            exit(EXIT_FAILURE);
        }
        addrSize = sizeof(struct sockaddr_in6);
    } else {
        // IPv4 address format
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        ((struct sockaddr_in *)&serverAddr)->sin_family = AF_INET;
        ((struct sockaddr_in *)&serverAddr)->sin_port = htons(atoi(argv[2]));
        if (inet_pton(AF_INET, argv[1], &((struct sockaddr_in *)&serverAddr)->sin_addr) <= 0) {
            perror("Invalid address");
            exit(EXIT_FAILURE);
        }
        addrSize = sizeof(struct sockaddr_in);
    }

    if (clientSocket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, addrSize) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");

    // Send the initial request message
    if (write(clientSocket, buffer, strlen(buffer)) < 0) {
        perror("Write failed");
        exit(EXIT_FAILURE);
    }

    // Send and receive messages
    while (1) {
        // Receive message from the server
        memset(buffer, 0, BUFFER_SIZE);
        if (read(clientSocket, buffer, BUFFER_SIZE) < 0) {
            perror("Read failed");
            exit(EXIT_FAILURE);
        }

        printf("Server message: %s\n", buffer);
        

        if(strcmp(buffer, "ERROR(NULL, 1)") == 0){
            printf("User limit exceeded\n");
            close(clientSocket);
            return 0;
        }else{
            IdOrigin = atoi(buffer);
            printf("This client ID is %d\n", IdOrigin);
        }

        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        printf("Enter a command:\n");
        fgets(buffer, BUFFER_SIZE, stdin); // User types a command




        // Remove newline character
        buffer[strcspn(buffer, "\n")] = '\0';

        // Send message to the server
        if (write(clientSocket, buffer, strlen(buffer)) < 0) {
            perror("Write failed");
            exit(EXIT_FAILURE);
        }
    }

    // Close the socket
    close(clientSocket);

    return 0;
}
