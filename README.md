# TCP MultiClient-Chat Suite

A high-performance, multi-threaded chat application built in **C** using **TCP Sockets**. This project facilitates real-time communication between multiple clients through a central server, operating entirely within the **Linux Terminal**.

## Contributors
* **Kalavagunta Saravanarjun** - Roll No: 23CS01028
* **Manimeli Shashikanth** - Roll No: 23CS01037

---

## How It Works
The application follows a client-server architecture designed for the Linux environment.

### The Server
* **Multi-threaded Handling**: The server uses the `pthread` library to create a unique thread for every connected client, allowing for concurrent communication.
* **Synchronization**: Implements `pthread_mutex_t` to protect shared resources, such as the client list, during dynamic joins and leaves.
* **Intelligent Routing**: Parses incoming packets to distinguish between broadcast messages and private peer-to-peer messages without using `strtok`.

### The Client
* **Concurrent I/O**: Utilizes dual threads—one for receiving incoming server data and another for capturing user terminal input—ensuring the UI never "freezes" while waiting for a message.
* **Graceful Termination**: Uses the `shutdown()` function to safely close socket connections before exiting.

---

### Requirements
* **Compiler**: GCC
* **Operating System**: Linux / Ubuntu / WSL
* **Libraries**: POSIX Threads (`pthread`)

### Compilation
Open your terminal and run the following commands:
```bash
# Compile the Server
gcc server.c -o server
# Compile the Client
gcc client.c -o client
```
### Execution
1. **Start the Server**: 
   ```bash
   ./server <port_number>
   ```
   *Example: `./server 7500`*

2. **Connect Clients**:
   ```bash
   ./client <server_ip> <port_number>
   ```
   *Example: `./client 10.10.34.25 7500`*

---

## 💬 Usage Instructions
Once connected, the terminal will display a menu for interaction:

| Option | Action        | Description                                                           |
| :---   | :---          | :---                                                                  |
| **0**  | **Broadcast** | Sends a message to every user currently connected to the server.      |
| **1**  | **Private**   | Sends a direct message to a specific user using their IP address.     |
| **2**  | **Exit**      | Safely disconnects the client from the server and closes the program. |

---

## 📝 Features
* **Real-time Interaction**: Instant message delivery across different terminal sessions.
* **Custom Labeling**: Messages are tagged as `[GROUP]` or `Message from client X` for clarity.
* **Thread Safety**: Mutex-based locking prevents race conditions during client management.
* **Dynamic Management**: Clients can join or leave the chat at any time without restarting the server.
