# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall

# Required libraries
AWS_LIBS = -laws-cpp-sdk-sqs -laws-cpp-sdk-core
THREAD_LIBS = -pthread

# Default target builds both executables
all: ChatServer ChatClient

# Server compilation (requires AWS SDK and pthreads)
ChatServer: main.cpp
	$(CXX) $(CXXFLAGS) -o ChatServer main.cpp $(AWS_LIBS) $(THREAD_LIBS)

# Client compilation (only requires pthreads)
ChatClient: client.cpp
	$(CXX) $(CXXFLAGS) -o ChatClient client.cpp $(THREAD_LIBS)

# Clean up compiled binaries
clean:
	rm -f ChatServer ChatClient