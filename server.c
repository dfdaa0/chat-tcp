#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_CLIENTS 15

void *handleClient(void *socket);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <v4 or v6> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_in6 serverAddrIPv6;
    socklen_t addrSize;
    pthread_t threads;

    if (strcmp(argv[1], "v4") == 0) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(atoi(argv[2]));
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
        if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Binding failed");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(argv[1], "v6") == 0) {
        serverSocket = socket(AF_INET6, SOCK_STREAM, 0);
        serverAddrIPv6.sin6_family = AF_INET6;
        serverAddrIPv6.sin6_port = htons(atoi(argv[2]));
        serverAddrIPv6.sin6_addr = in6addr_any;
        serverAddrIPv6.sin6_flowinfo = 0;
        serverAddrIPv6.sin6_scope_id = 0;
        if (bind(serverSocket, (struct sockaddr *)&serverAddrIPv6, sizeof(serverAddrIPv6)) < 0) {
            perror("Binding failed");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Invalid IP version specified\n");
        exit(EXIT_FAILURE);
    }

    // Listen for client connections
    if (listen(serverSocket, SOMAXCONN) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %s...\n", argv[2]);

    // Accept and handle client connections
    while (1) {
        addrSize = sizeof(struct sockaddr_storage);
        struct sockaddr_storage clientAddrStorage;
        struct sockaddr *clientAddr = (struct sockaddr *)&clientAddrStorage;

        // Accept a new client connection
        newSocket = accept(serverSocket, clientAddr, &addrSize);
        if (newSocket < 0) {
            perror("Accepting connection failed");
            exit(EXIT_FAILURE);
        }

        printf("New client connected\n");

        // Create a new thread to handle the client
        if (pthread_create(&threads, NULL, handleClient, (void *)&newSocket) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

typedef struct {
    int id;
    int socket;
} ClientInfo;

int numClients = 0;
ClientInfo clients[MAX_CLIENTS];


void *handleClient(void *arg) {
    int clientSocket = *(int *)arg;
    char buffer[1024] = {0};
    int bytesRead;

    // Receive and handle client messages
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        printf("Received message from client: %s\n", buffer);

        if (strcmp(buffer, "REQ_ADD") == 0) {
            if (numClients >= MAX_CLIENTS) {
                // Maximum number of clients reached
                printf("Sending ERROR(1) to the client\n");
                const char *message = "ERROR(1)";
                send(clientSocket, message, strlen(message), 0);
            } else {
                // Assign an ID to the client
                int clientId = numClients++;
                clients[clientId].id = clientId;
                clients[clientId].socket = clientSocket;

                // Send the assigned ID to the client
                char idMessage[50];
                sprintf(idMessage, "%d\n", clientId); // Send the Id number to the client
                printf("Client %d added\n", clientId);
                send(clientSocket, idMessage, strlen(idMessage), 0);
            }
        } 
        
        else if (strncmp(buffer, "REQ_REM", 7) == 0) {
            // Process REQ_REM message
            int idSender;
            sscanf(buffer, "REQ_REM(%d)", &idSender);

            // Remove the client with idSender from the network
            // ...


        } 
        
        else if (strncmp(buffer, "RES_LIST", 8) == 0) {
            // Process RES_LIST message
            // ...


        } 
        
        else if (strncmp(buffer, "MSG", 3) == 0) {
            // Process MSG message
            int idSender, idReceiver;
            char message[1024];
            sscanf(buffer, "MSG(%d,%d,%[^\n])", &idSender, &idReceiver, message);

            // Check if the receiver exists in the network
            // ...

            // Forward the message to the receiver's client
            // ...

            // Print error message if receiver not found
            // ...
        } 
        
        else {
            // Unknown message
            // ...
        }

        memset(buffer, 0, sizeof(buffer));
    }

    if (bytesRead == 0) {
        printf("Client disconnected\n");
    } else if (bytesRead == -1) {
        perror("Receive error");
    }

    // Remove the client from the list
    for (int i = 0; i < numClients; i++) {
        if (clients[i].socket == clientSocket) {
            // Shift the remaining clients to fill the gap
            for (int j = i; j < numClients - 1; j++) {
                clients[j] = clients[j + 1];
            }
            numClients--;
            break;
        }
    }

    // Close the client socket
    close(clientSocket);

    pthread_exit(NULL);
}
