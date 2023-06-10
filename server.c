#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_CLIENTS 15

void *handleClient(void *socket);

typedef struct {
    int id;
    int socket;
} ClientInfo;

int numClients = 0;
ClientInfo clients[MAX_CLIENTS];

// Initialize clients array
for (int i = 0; i < MAX_CLIENTS; i++) {
    clients[i].id = -1;
    clients[i].socket = -1;
}

pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;

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

void sendMessageToClient(int clientSocket, const char *message) {
    send(clientSocket, message, strlen(message), 0);
}

void sendMessageToAllClients(const char *message) {
    pthread_mutex_lock(&clientsMutex);

    for (int i = 0; i < numClients; i++) {
        send(clients[i].socket, message, strlen(message), 0);
    }

    pthread_mutex_unlock(&clientsMutex);
}

void removeClient(int clientSocket) {
    pthread_mutex_lock(&clientsMutex);

    for (int i = 0; i < numClients; i++) {
        if (clients[i].socket == clientSocket) {
            printf("User %d removed\n", clients[i].id);

            // Send the removal message to all other clients
            char removalMessage[50];
            sprintf(removalMessage, "User %d removed\n", clients[i].id);
            sendMessageToAllClients(removalMessage);

            // Shift the remaining clients to fill the gap
            for (int j = i; j < numClients - 1; j++) {
                clients[j] = clients[j + 1];
            }
            numClients--;

            // Freeing the ID
            clients[i].id = -1;
            break;
        }
    }

    pthread_mutex_unlock(&clientsMutex);
}

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
                sendMessageToClient(clientSocket, message);
            } else {
                // Find the first available ID
                int clientId = -1;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket == -1) {
                        clientId = i;
                        break;
                    }
                }

                if (clientId == -1) {
                    // No available ID found (shouldn't happen if MAX_CLIENTS is correct)
                    printf("For some reason, no avaliable ID was found\n");
                } else {
                    clients[clientId].id = clientId;
                    clients[clientId].socket = clientSocket;

                    // Send the assigned ID to the client
                    char idMessage[50];
                    sprintf(idMessage, "You are the number %d\n", clientId);
                    printf("Client %d added\n", clientId);
                    sendMessageToClient(clientSocket, idMessage);
                }
            }
        } else if (strncmp(buffer, "REQ_REM", 7) == 0) {
            // Process REQ_REM message
            int idSender;
            sscanf(buffer, "REQ_REM(%d)", &idSender);

            // Remove the client with idSender from the network
            removeClient(clientSocket);

        } else if (strncmp(buffer, "RES_LIST", 8) == 0) {
            // Process RES_LIST message
            // Build the list of connected client IDs
            char listMessage[1024] = {0};
            pthread_mutex_lock(&clientsMutex);
            for (int i = 0; i < numClients; i++) {
                char idString[10];
                sprintf(idString, "%d ", clients[i].id);
                strcat(listMessage, idString);
            }
            pthread_mutex_unlock(&clientsMutex);

            // Send the list to the client
            sendMessageToClient(clientSocket, listMessage);

        } else if (strncmp(buffer, "MSG", 3) == 0) {
            // Process MSG message
            int idSender, idReceiver;
            char message[1024];
            sscanf(buffer, "MSG(%d,%d,%[^\n])", &idSender, &idReceiver, message);

            // Check if the receiver exists in the network
            int receiverSocket = -1;
            pthread_mutex_lock(&clientsMutex);
            for (int i = 0; i < numClients; i++) {
                if (clients[i].id == idReceiver) {
                    receiverSocket = clients[i].socket;
                    break;
                }
            }
            pthread_mutex_unlock(&clientsMutex);

            if (receiverSocket != -1) {
                // Forward the message to the receiver's client
                char forwardMessage[1100];
                sprintf(forwardMessage, "MSG(%d,%d,%s)", idSender, idReceiver, message);
                send(receiverSocket, forwardMessage, strlen(forwardMessage), 0);
            } else {
                // Print error message if receiver not found
                printf("User %d not found\n", idReceiver);
            }
        } else {
            // Unknown message
            // ...
        }

        memset(buffer, 0, sizeof(buffer));
    }

    if (bytesRead == 0) {
        printf("Client disconnected\n");
        removeClient(clientSocket);
    } else if (bytesRead == -1) {
        perror("Receive error");
    }

    // Close the client socket
    close(clientSocket);

    pthread_exit(NULL);
}
