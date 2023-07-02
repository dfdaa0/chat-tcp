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
        default:
            printf("Unknown error\n");
            break;
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

    // Receive response from the server
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(clientSocket, buffer, BUFFER_SIZE, 0) < 0) {
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }

    if (strncmp(buffer, "ERROR(1)", 8) == 0) {
        printf("Connection rejected by the server. Closing the program.\n");
        close(clientSocket);
        exit(EXIT_FAILURE);
    } else {
        printf("Connection accepted by the server.\n");
        int IdOrigin = atoi(buffer);
        printf("IdOrigin: %d\n", IdOrigin);
    }

    // Send and receive messages
    while (1) {
        printf("Enter a command: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Remove newline character
        buffer[strcspn(buffer, "\n")] = '\0';

        // Process user command
        if (strcmp(buffer, "list users") == 0) {
            // Send "RES_LIST[IdOrigin]" to the server
            sprintf(buffer, "RES_LIST%d", IdOrigin);
        } else if (strncmp(buffer, "send to ", 8) == 0) {
            // Extract IdReceiver and Message from the command
            int IdReceiver;
            char Message[BUFFER_SIZE];
            sscanf(buffer, "send to %d \"%[^\"]\"", &IdReceiver, Message);

            // Send "MSG([IdOrigin], [IdReceiver], [Message])" to the server
            sprintf(buffer, "MSG(%d, %d, %s)", IdOrigin, IdReceiver, Message);
        } else if (strncmp(buffer, "send all", 8) == 0) {
            // Extract Message from the command
            char Message[BUFFER_SIZE];
            sscanf(buffer, "send all \"%[^\"]\"", Message);

            // Send "MSG([IdOrigin], NULL, [Message])" to the server
            sprintf(buffer, "MSG(%d, NULL, %s)", IdOrigin, Message);
        } else if (strcmp(buffer, "close connection") == 0) {
            // Send "REQ_REM([IdOrigin])" to the server
            sprintf(buffer, "REQ_REM(%d)", IdOrigin);
        } else {
            printf("Invalid command\n");
            continue;
        }

        // Send command to the server
        if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        // Receive response from the server
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
    }

    // Close the socket
    close(clientSocket);

    return 0;
}
