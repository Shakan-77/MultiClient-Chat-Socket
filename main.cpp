#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <aws/core/Aws.h>
#include <aws/sqs/SQSClient.h>
#include <aws/sqs/model/SendMessageRequest.h>

constexpr int PORT = 8080;
constexpr int MAX_CLIENTS = 50;

std::mutex clients_mutex;
std::vector<int> active_clients; // Track all connected socket descriptors

// Thread-safe broadcast to all clients except the sender
void broadcast_message(const std::string& message, int sender_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client_fd : active_clients) {
        if (client_fd != sender_socket) {
            send(client_fd, message.c_str(), message.length(), 0);
        }
    }
}

void send_private_message(const std::string& message, int sender_socket, int target_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    // Check if the target user is currently connected
    if (std::find(active_clients.begin(), active_clients.end(), target_socket) != active_clients.end()) {
        if(target_socket==sender_socket){
            std::string err = "[System]: Cannot send a message to yourself.\n";
            send(sender_socket, err.c_str(), err.length(), 0);
        }
        else{
            send(target_socket, message.c_str(), message.length(), 0);
        }
    } else {
        std::string err = "[System]: User " + std::to_string(target_socket) + " is not connected.\n";
        send(sender_socket, err.c_str(), err.length(), 0);
    }
}

void handle_client(int client_socket) {
    // Register the new client
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        active_clients.push_back(client_socket);
    }
    
    std::cout << "Client connected on socket " << client_socket 
              << " (Thread: " << std::this_thread::get_id() << ")\n";
    
    char buffer[1024];
    
    // Persistent Read Loop
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            std::cout << "Client on socket " << client_socket << " disconnected.\n";
            break; // Exit loop on disconnect
        }
        
        std::string msg(buffer);
        // Strip trailing newlines or whitespace from the raw socket buffer
        msg.erase(msg.find_last_not_of(" \n\r\t") + 1);

        // Detect the private message command format: /msg <target_id> <message>
        if (msg.rfind("/msg ", 0) == 0) { 
            size_t first_space = 4;
            size_t second_space = msg.find(' ', first_space + 1);
            
            if (second_space != std::string::npos) {
                try {
                    int target_fd = std::stoi(msg.substr(first_space + 1, second_space - first_space - 1));
                    std::string private_text = "[Private from " + std::to_string(client_socket) + "]: " + msg.substr(second_space + 1);
                    
                    send_private_message(private_text, client_socket, target_fd);
                } catch (...) {
                    std::string err = "[System]: Invalid format. Use /msg <user_id> <message>\n";
                    send(client_socket, err.c_str(), err.length(), 0);
                }
            }
        } else {
            // Standard public broadcast for all other messages
            std::string formatted_msg = "[User " + std::to_string(client_socket) + "]: " + msg;
            broadcast_message(formatted_msg, client_socket);
        }
        
        //Asynchronously push metadata to AWS (Fails gracefully until account is ready)
        std::string jsonPayload = "{\"userId\": \"user_" + std::to_string(client_socket) + "\", \"length\": " + std::to_string(bytes_read) + "}";
        
        Aws::Client::ClientConfiguration clientConfig;
        clientConfig.region = "us-east-1";
        Aws::SQS::SQSClient sqsClient(clientConfig);
        Aws::SQS::Model::SendMessageRequest request;
        request.SetQueueUrl("https://sqs.us-east-1.amazonaws.com/000000000000/example-queue");
        request.SetMessageBody(jsonPayload);
        
        auto outcome = sqsClient.SendMessage(request);
        if (!outcome.IsSuccess()) {
            std::cerr << "[AWS Pending] SQS push skipped: " << outcome.GetError().GetMessage() << "\n";
        }
    }
    
    // Clean up on disconnect
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        active_clients.erase(std::remove(active_clients.begin(), active_clients.end(), client_socket), active_clients.end());
    }
    close(client_socket);
}

int main() {
    // 1. Initialize the AWS SDK before making any socket or AWS calls
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    std::cout << "AWS SDK Initialized.\n";

    // 2. Setup the standard POSIX socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    // Allow immediate reuse of the port to prevent binding errors during rapid restarts
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "C++ Chat Server with AWS SQS integration listening on port " << PORT << "...\n";

    std::vector<std::thread> client_threads;

    // 3. Main Accept Loop
    while (true) {
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);
        
        int new_socket = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
        if (new_socket < 0) {
            std::cerr << "Accept failed\n";
            continue;
        }

        // 4. Thread-safe client registration using C++17 paradigms
        std::lock_guard<std::mutex> lock(clients_mutex);
        client_threads.emplace_back(std::thread(handle_client, new_socket));
        
        // Detach thread to allow independent asynchronous execution 
        client_threads.back().detach(); 
    }

    // 5. Clean up AWS resources (placed here for architectural completeness)
    Aws::ShutdownAPI(options);
    return 0;
}