# Multithreaded Proxy Server in C

A simple multithreaded proxy server built in C to deepen understanding of C, networking, and HTTP😁

## Features
- Supports **HTTP/1.1** protocol
- Handles multiple concurrent requests using **multithreading**
- Implements **basic request forwarding** with support for **libcurl** and **llhttp**

## How to Run Locally

### 1. Compile and Start the Proxy Server
```sh
gcc main.c -o main -I/usr/include/x86_64-linux-gnu -L/usr/local/lib -lllhttp -lcurl
./main
```

### 2. Run the Client to Send Requests
Open a new terminal and execute:
```sh
chmod +x multiclientreqs.sh && ./multiclientreqs.sh
```