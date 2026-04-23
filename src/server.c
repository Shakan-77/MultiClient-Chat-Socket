#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAXCLIENTS 200

// Structure to hold client information(socket number to IP address mapping)

typedef struct {
    int sockfd;
    char ip[INET_ADDRSTRLEN];
} Client;

Client clients[MAXCLIENTS]; // Array to hold connected clients
int numclients = 0;

pthread_mutex_t lock; // Mutex to protect access to clients array and numclients variable

void error(char *msg){
    perror(msg);
    exit(1);
}

// Function to broadcast a message to all clients except the sender
void broadcast(char *msg, int sender){
    pthread_mutex_lock(&lock);

    char final_msg[1200];
    snprintf(final_msg, sizeof(final_msg),
             "[GROUP] Client %d: %s", sender, msg);

    for(int i = 0; i < numclients; i++){
        if(clients[i].sockfd != sender){
            write(clients[i].sockfd, final_msg, strlen(final_msg));
        }
    }

    pthread_mutex_unlock(&lock);
}

// Function to send a message to a specific client based on IP address

void send_to_ip(char *ip, char *msg, int sender){
    pthread_mutex_lock(&lock);

    char final_msg[1200];
    snprintf(final_msg, sizeof(final_msg),
             "Message from client %d: %s", sender, msg);

    for(int i = 0; i < numclients; i++){
        if(strcmp(clients[i].ip, ip) == 0){
            write(clients[i].sockfd, final_msg, strlen(final_msg));
            pthread_mutex_unlock(&lock);
            return;
        }
    }

    pthread_mutex_unlock(&lock);
}

// Thread function to handle communication with a client

void *handle_client(void *arg){
    int newsockfd = *(int *)arg;
    free(arg);

    while(1){
        char buffer[1024];
        bzero(buffer, sizeof(buffer));

        int n = read(newsockfd, buffer, sizeof(buffer) - 1);

        if(n <= 0){
            break;
        }

        buffer[n] = '\0';

        // Check if message is a broadcast (indicated by special prefix "256.256.256.256:") or a private message (in format "IP:message")

        if(strncmp(buffer, "256.256.256.256:", 16) == 0){
            // Broadcast message
            char *msg = buffer + 16;
            broadcast(msg, newsockfd);
        }
        else{
            char *colon = strchr(buffer, ':'); // Find the colon that separates IP and message

            if(colon != NULL){
                *colon = '\0';
                char *ip = buffer;
                char *msg = colon + 1; // Message is everything after the colon
                send_to_ip(ip, msg, newsockfd);
            }
        }
    }

    pthread_mutex_lock(&lock); // Remove client from clients array when they disconnect
    for(int i = 0; i < numclients; i++){
        if(clients[i].sockfd == newsockfd){
            clients[i] = clients[numclients - 1];
            numclients--;
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    close(newsockfd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr,"Usage: %s port\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&lock, NULL);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        error("ERROR opening socket");

    struct sockaddr_in serv_addr, cli_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));

    int portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);

    socklen_t clilen = sizeof(cli_addr);

    while(1){
        int *newsockfd = malloc(sizeof(int));
        *newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (*newsockfd < 0)
            error("ERROR on accept");

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli_addr.sin_addr, ip, INET_ADDRSTRLEN);

        pthread_mutex_lock(&lock); // Add new client to clients array
        clients[numclients].sockfd = *newsockfd;
        strcpy(clients[numclients].ip, ip);
        numclients++;
        pthread_mutex_unlock(&lock);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, newsockfd); // Start thread to handle communication with this client
        pthread_detach(tid);
    }

    close(sockfd);
    return 0;
}