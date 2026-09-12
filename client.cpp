#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int PORT = 8080;

// Background thread to continuously read incoming broadcast messages
void receive_messages(int socket_fd) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(socket_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            std::cout << "\nDisconnected from server." << std::endl;
            exit(0);
        }
        // Clear the current input line, print the message, and restore the prompt
        std::cout << "\r" << buffer << "\nYou: " << std::flush;
    }
}

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    if (connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Connection to server failed" << std::endl;
        return 1;
    }

    std::cout << "Connected to Chat Server. Type your messages, '/msg <user_id> <message>' for a private message, or '/quit' to exit." << std::endl;
    
    // Launch receive thread
    std::thread receiver(receive_messages, client_fd);
    receiver.detach();

    // Main thread handles sending
    std::string message;
    while (true) {
        std::cout << "You: ";
        std::getline(std::cin, message);
        
        if (message == "/quit") {
            break;
        }
        
        if (!message.empty()) {
            send(client_fd, message.c_str(), message.length(), 0);
        }
    }

    close(client_fd);
    return 0;
}