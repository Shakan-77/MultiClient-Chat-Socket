# hpc-chat-app

## About the Project

`hpc-chat-app` is a real-time command-line chat application built as a fourth-year student portfolio project. The C++ server manages socket connections and chat messages, while a Python background worker processes message metadata through AWS SQS and stores it in DynamoDB.

## Tech Stack

- C++17 for the TCP socket server and CLI client
- Python and FastAPI for the background worker
- AWS SQS for passing message metadata
- Amazon DynamoDB for storing processed metadata
- Docker Compose for running the services together
- WSL for local Linux development

## How to Run

1. Clone the repository and move into the project directory:

	```bash
	git clone https://github.com/Shakan-77/MultiClient-Chat-Socket.git
	cd MultiClient-Chat-Socket
	```

2. Create a local environment file from the example:

	```bash
	cp .env.example .env
	```

3. Open `.env` and replace the example values with your AWS credentials, region, SQS queue URL, and DynamoDB table name. Keep this file private. It is excluded by `.gitignore` and must not be committed.

4. Build and start the server and worker:

	```bash
	docker compose up --build
	```

The C++ chat server listens on port `8080`. The FastAPI worker is available on port `8000`. To stop the services, press `Ctrl+C`.

## Known Limitations & Future Improvements

The C++ server is currently stateful: active client connections are held in the memory of one server instance. To scale horizontally across multiple server instances, the project would need a Pub/Sub layer such as Redis to share connection and message events between instances.