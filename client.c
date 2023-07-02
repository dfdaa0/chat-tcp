#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void printErrorMessage(int code) {
    switch (code) {
        case 1:
            printf("User limit exceeded\n");
            break;
        case 2:
            printf("User not found\n");
            break;
        case 3:
            printf("Receiver not found\n");
            break;
        default:
            printf("Unknown error\n");
            break;
    }
}

void handleMessage(char *message) {
    if (strncmp(message, "RES_LIST", 8) == 0) {
        // Parse the user IDs from the message
        char *list = strtok(message, "()");
        char *ids = strtok(NULL, "()");
        printf("Available users: %s\n", ids);
    } else if (strncmp(message, "MSG", 3) == 0) {
        // Parse the sender ID and message from the message
        char *sender = strtok(message, "()");
        char *msg = strtok(NULL, "()");
        printf("%s: %s\n", sender, msg);
    } else if (strncmp(message, "REQ_REM", 7) == 0) {
        // Parse the user ID to be removed
        char *userId = strtok(message, "()");
        printf("User %s has been removed from the network\n", userId);
    } else if (strncmp(message, "ERROR", 5) == 0) {
        // Parse the receiver ID and error code from the message
        char *receiver = strtok(message, "()");
        char *errorCode = strtok(NULL, "()");
        int code = atoi(errorCode);
        printf("Error message for user %s: ", receiver);
        printErrorMessage(code);
    } else if (strncmp(message, "OK", 2) == 0) {
        // Parse the receiver ID and success code from the message
        char *receiver = strtok(message, "()");
        char *successCode = strtok(NULL, "()");
        int code = atoi(successCode);
        printf("Success message for user %s: ", receiver);
        if (code == 1) {
            printf("Removed Successfully\n");
        } else {
            printf("Unknown code\n");
        }
    } else {
        printf("Unknown message received from the server\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server IP> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int clientSocket;
    struct sockaddr_storage serverAddr;
    socklen_t addrSize;
    char buffer[BUFFER_SIZE];

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
    if (send(clientSocket, "REQ_ADD", 7, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    // Receive and send messages
    while (1) {
        // Receive message from the server
        memset(buffer, 0, BUFFER_SIZE);
        if (recv(clientSocket, buffer, BUFFER_SIZE, 0) < 0) {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }

        printf("Server message: %s\n", buffer);

        // Check for exit command
        if (strcmp(buffer, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }

        // Process the received message
        handleMessage(buffer);

        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);

        // Get user input
        printf("Enter a command: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Remove newline character
        buffer[strcspn(buffer, "\n")] = '\0';

        // Process user command
        if (strcmp(buffer, "list users") == 0) {
            // Request the list of user IDs from the server
            sprintf(buffer, "RES_LIST");
        } else if (strncmp(buffer, "send to", 7) == 0) {
            // Parse the receiver ID and message from the command
            int receiverId;
            char *message;
            sscanf(buffer, "send to %d \"%[^\"]\"", &receiverId, message);
            // Send message to the server
            sprintf(buffer, "MSG(%d,%d,%s)", receiverId, receiverId, message);
        } else if (strncmp(buffer, "send all", 8) == 0) {
            // Parse the message from the command
            char *message;
            sscanf(buffer, "send all \"%[^\"]\"", message);
            // Send message to the server
            sprintf(buffer, "MSG(%d,NULL,%s)", receiverId, message);
        } else if (strcmp(buffer, "close connection") == 0) {
            // Request disconnection from the server
            sprintf(buffer, "REQ_REM(%d)", 0);  // Replace 0 with the actual user ID
        } else {
            printf("Unknown command\n");
            continue;
        }

        // Send message to the server
        if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }
    }

    // Close the socket
    close(clientSocket);

    return 0;
}
