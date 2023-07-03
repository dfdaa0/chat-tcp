#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

int IdOrigin = -1;
int clientSocket;
char buffer[BUFFER_SIZE];

void *readThread(void *arg) {
    while (1) {
        // Receive message from the server
        memset(buffer, 0, BUFFER_SIZE);
        if (recv(clientSocket, buffer, BUFFER_SIZE, 0) < 0) {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }

        printf("Server message: %s\n", buffer);

        if (strcmp(buffer, "ERROR(NULL, 1)") == 0) {
            printf("User limit exceeded\n");
            close(clientSocket);
            exit(EXIT_SUCCESS);
        } else if (strncmp(buffer, "ID(", 3) == 0) {
            int id;
            if (sscanf(buffer, "ID(%d)", &id) == 1) {
                IdOrigin = id;
                printf("This client ID is %d\n", IdOrigin);
            }
        } else {
            if(recv(clientSocket, buffer, BUFFER_SIZE, 0) == 0){
                printf("Connection closed\n");
                exit(0);
            }
                
        }
    }
}

void *writeThread(void *arg) {
    while (1) {
        // printf("Enter a command:\n");
        fgets(buffer, BUFFER_SIZE, stdin); // User types a command

        // Remove newline character
        buffer[strcspn(buffer, "\n")] = '\0';

        // Switch case para lidar com os comandos
        if (strncmp(buffer, "list users", BUFFER_SIZE) == 0) {
            snprintf(buffer, BUFFER_SIZE, "RES_LIST(%d)", IdOrigin);
        } else if (strncmp(buffer, "send to", 7) == 0) {
            int IdReceiver;
            char Message[BUFFER_SIZE];

            if (sscanf(buffer, "send to %d \"%[^\"]\"", &IdReceiver, Message) != 2) {
                printf("Invalid command format.\n");
                continue; // Retorna ao início do loop
            }

            // Adiciona aspas duplas ao redor de Message
            char MessageWithQuotes[BUFFER_SIZE];
            snprintf(MessageWithQuotes, BUFFER_SIZE, "\"%s\"", Message);

            snprintf(buffer, BUFFER_SIZE, "MSG(%d, %d, %s)", IdOrigin, IdReceiver, MessageWithQuotes);
        } else if (strncmp(buffer, "send all", 8) == 0) {
            char Message[BUFFER_SIZE];

            if (sscanf(buffer, "send all \"%[^\"]\"", Message) != 1) {
                printf("Invalid command format.\n");
                continue; // Retorna ao início do loop
            }

            // Adiciona aspas duplas ao redor de Message
            char MessageWithQuotes[BUFFER_SIZE];
            snprintf(MessageWithQuotes, BUFFER_SIZE, "\"%s\"", Message);

            snprintf(buffer, BUFFER_SIZE, "MSG(%d, NULL, %s)", IdOrigin, MessageWithQuotes);

        } else if (strncmp(buffer, "close connection", BUFFER_SIZE) == 0) {
            snprintf(buffer, BUFFER_SIZE, "REQ_REM(%d)", IdOrigin);

        } else {
            printf("Invalid command.\n");
            continue; // Retorna ao início do loop
        }

        // Send message to the server
        if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server IP> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage serverAddr;
    socklen_t addrSize;

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
    strcpy(buffer, "REQ_ADD");
    if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    pthread_t readThreadId, writeThreadId;

    // Create read thread
    if (pthread_create(&readThreadId, NULL, readThread, NULL) != 0) {
        perror("Read thread creation failed");
        exit(EXIT_FAILURE);
    }

    // Create write thread
    if (pthread_create(&writeThreadId, NULL, writeThread, NULL) != 0) {
        perror("Write thread creation failed");
        exit(EXIT_FAILURE);
    }

    // Wait for threads to finish
    pthread_join(readThreadId, NULL);
    pthread_join(writeThreadId, NULL);

    // Close the socket
    close(clientSocket);

    return 0;
}
