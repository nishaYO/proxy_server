#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>

#include "parse.h"
#include "network.h"

int proxy_fd;

// Signal handler function for SIGINT
void handle_sigint(int sig) {
    printf("\nCaught SIGINT (Ctrl+C). Shutting down the server...\n");

    // Close the proxy socket
    if (proxy_fd >= 0) {
        close(proxy_fd);
        printf("Proxy socket closed.\n");
    }

    exit(0); // Exit the program gracefully
}

// Function prototypes
http_request_t *parse_http1_request(char *request_buffer);
char *forwardReqToTarget(http_request_t *req);
// int sendResponseToClient(int client_fd, char *response);

int main() {
    // Register the SIGINT handler
    signal(SIGINT, handle_sigint);

    // Create a socket
    proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_fd == -1){
        perror("SOCKET FILE DESC");
        exit(EXIT_FAILURE);
    }

    // Address info 
    struct sockaddr_in proxy_addr, client_addr;
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(8080);
    proxy_addr.sin_addr.s_addr = INADDR_ANY;

    socklen_t client_addr_len = sizeof(client_addr);

    // Bind socket with address
    if(bind(proxy_fd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) != 0 ){
        perror("BIND");
        close(proxy_fd);
        exit(EXIT_FAILURE);
    }

    // Listen from the socket endpoint 
    if(listen(proxy_fd, 5) != 0){
        perror("LISTEN");
        close(proxy_fd);
        exit(EXIT_FAILURE);
    }

    printf("Proxy server is running. Press Ctrl+C to stop.\n");

    while(1){
        // Accept the client request 
        int client_fd = accept(proxy_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if(client_fd < 0){
            perror("ACCEPT");
            continue;
        }
 
        printf("Accepted client request.\n");

        // Buffer initialize to store client request data
        char request_buffer[1024];
        size_t request_buffer_len = sizeof(request_buffer);

        // Receive data from client socket 
        int bytes_received = recv(client_fd, request_buffer, request_buffer_len, 0);
        if(bytes_received < 0){
            perror("RECV");
            printf("Failed to receive data, bytes_received = %d\n", bytes_received);
            close(client_fd);
            continue;
        }

        if (bytes_received >= sizeof(request_buffer)) {
            fprintf(stderr, "Request too large for buffer from recv.\n");
            close(client_fd);
            continue;
        }

        request_buffer[bytes_received] = '\0'; // null terminate the request_buffer to make a valid string
        printf("Request Buffer: \n%s\n-----------------\n", request_buffer);

        // Parse from request
        http_request_t *req = parse_http1_request(request_buffer);
        if (!req) {
            fprintf(stderr, "Failed to parse HTTP request\n");
            close(client_fd);
            continue;
        }

        // Forward req to the target server and get response 
        char *response = forwardReqToTarget(req);
        if (!response){
            fprintf(stderr, "Failed to forward Request To Target.\n");
            free(req);
            close(client_fd);
            continue;
        }
        printf("response from target: %s\n", response);
        // // Send the response to the client 
        // if (sendResponseToClient(client_fd, response) != 0) {
        //     fprintf(stderr, "Error sending response to client\n");
        // }

        // Cleanup
        if (req) {
            if (req->headers) free(req->headers);
            if (req->body) free(req->body);
            free(req);  // Free the main structure
        }
        free(response);
        shutdown(client_fd, SHUT_RDWR); // Stop reading/writing on the socket
        close(client_fd);  // Close the client socket
    }

    close(proxy_fd);
    return 0;
}