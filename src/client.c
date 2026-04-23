#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

int sockfd;
volatile int running = 1; // Whether the client is still running or nott

void error(char *msg){
    perror(msg);
    exit(1);
}

// Thread function to send messages to the server

void *send_msg(void *arg){
    char buffer[BUFFER_SIZE];

    while(running){
        int inp;
        printf("\nEnter 0 (broadcast), 1 (private), 2 (exit): "); //Menu for user input
        scanf("%d", &inp);
        getchar();

        switch(inp){
            // If user chooses to exit, close the connection and stop the client
            case 2:
                printf("Leaving chat...\n");
                running = 0;
                shutdown(sockfd, SHUT_RDWR);
                close(sockfd);
                break;

            // If user chooses to broadcast, send message with special prefix to indicate broadcast
            case 0:
                printf("Enter message: ");
                fgets(buffer, BUFFER_SIZE, stdin);

                buffer[strcspn(buffer, "\n")] = 0; 

                char final_msg[1200];
                snprintf(final_msg, sizeof(final_msg), "256.256.256.256:%s", buffer);

                write(sockfd, final_msg, strlen(final_msg));

            // If user chooses to send a private message, ask for receiver IP and message, then send in format "IP:message"
            case 1:
            char ipaddr[50];

            printf("Enter receiver IP: ");
            fgets(ipaddr, sizeof(ipaddr), stdin);
            ipaddr[strcspn(ipaddr, "\n")] = 0;

            printf("Enter message: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = 0;

            char final_msg[1200];
            snprintf(final_msg, sizeof(final_msg), "%s:%s", ipaddr, buffer);

            write(sockfd, final_msg, strlen(final_msg));

            default :
                printf("Invalid input. Try again.\n");
        }
    }

    pthread_exit(NULL);
}

// Thread function to receive messages from the server and print them to the console

void *recv_msg(void *arg){
    char buffer[BUFFER_SIZE];
    int n;

    while(running){
        bzero(buffer, BUFFER_SIZE);
        n = read(sockfd, buffer, BUFFER_SIZE - 1); // Read message from server

        if(n <= 0){
            printf("\nDisconnected from server\n");
            running = 0;
            close(sockfd);
            break;
        }

        buffer[n] = '\0';
        printf("\n[Message]: %s\n", buffer);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]){
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if(argc < 3){
        fprintf(stderr,"Usage: %s hostname port\n", argv[0]);
        exit(1);
    }

    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if(server == NULL){
        fprintf(stderr,"ERROR, no such host\n");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);

    serv_addr.sin_port = htons(portno);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    printf("Connected to server...\n");

    pthread_t send_thread, recv_thread;

    pthread_create(&send_thread, NULL, send_msg, NULL); // Start thread to send messages to server
    pthread_create(&recv_thread, NULL, recv_msg, NULL); // Start thread to receive messages from server

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    close(sockfd);
    return 0;
}