# MultiClient Chat Socket

A Linux/WSL chat application with a thread-safe C++17 TCP server, a custom C++ client, and a Python FastAPI pipeline that consumes Amazon SQS messages and stores records in DynamoDB.

## Components

- `main.cpp`: multi-client TCP server with per-client threads and mutex-protected client management.
- `client.cpp`: terminal client with concurrent input and receive handling.
- `main.py`: FastAPI service that polls SQS and writes message metadata to DynamoDB.
- `CMakeLists.txt` and `Makefile`: C++17 build configuration.

## Requirements

- Linux or WSL
- C++17 compiler, CMake, Make, and POSIX threads
- AWS SDK for C++ with the SQS component
- Python 3.10+ and the packages in `requirements.txt`
- AWS credentials and resource settings in a local `.env` file

Copy `.env.example` to `.env` and fill in the values. Never commit `.env` or AWS credentials.

## Build the C++ chat application

```bash
cmake -S . -B build
cmake --build build
```

This produces `build/ChatServer` and `build/ChatClient`, which are intentionally ignored by Git.

Start the server and then connect one or more clients from separate terminals using the command-line prompts exposed by the current source.

## Run the Python pipeline

```bash
python3 -m venv venv
source venv/bin/activate
python -m pip install -r requirements.txt
uvicorn main:app --reload
```

The service starts an asynchronous SQS consumer at application startup and stores each processed message's ID, user ID, and length in the configured DynamoDB table.

## Security

The repository ignores `.env`, compiled binaries, build output, virtual environments, and Python cache files. Use AWS IAM credentials appropriate for the deployment environment rather than storing secrets in source control.