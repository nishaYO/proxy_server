#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

// Extract host name (target server) from the client request
char *extractTargetHost(char request_buffer[]){
    char *host_header_start = strstr(request_buffer, "Host: ");
    if (host_header_start == NULL) {
        perror("Host header not found.");
        return NULL;
    }

    char *host_line_copy = strdup(host_header_start);
    if(host_line_copy==NULL){
        perror("Could not allocate memory to host_line_copy.");
        return NULL;
    }

    char *host_name = strtok(host_line_copy+6, "\n"); // converts \n to \0
    char *host_name_copy = NULL;
    if(host_name == NULL){
        perror("Failed to parse the hostname");
    } else {
        host_name_copy = strdup(host_name);
    }

    free(host_line_copy);
    return host_name_copy;
}


int main() {
    // Create a socket
    int proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_fd == -1){
        perror("SOCKET FILE DESC");
        exit(EXIT_FAILURE);
    }

    // Address info
    struct sockaddr_in proxy_addr, client_addr;
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(8080);
    proxy_addr.sin_addr.s_addr = INADDR_ANY;

    unsigned int client_addr_len = (unsigned int)sizeof(client_addr);

    // Bind socket with address
    if(bind(proxy_fd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) != 0 ){
        perror("BIND");
        exit(EXIT_FAILURE);
    }

    // Listen from the socket endpoint 
    if(listen(proxy_fd, 5) != 0){
        perror("LISTEN");
        exit(EXIT_FAILURE);
    }

    while(1){
        // Accept the client request 
        int client_fd = accept(proxy_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if(client_fd < 0){
            perror("ACCEPT");
            exit(EXIT_FAILURE);
        }
 
        printf("Accepted client request.\n");

        // Buffer initialize to store client request data
        char request_buffer[1024];
        long request_buffer_len = sizeof(request_buffer);

        // Receive data from client socket 
        int bytes_received = recv(client_fd, request_buffer, request_buffer_len, 0);
        if(bytes_received < 0){
            perror("RECV");
            close(client_fd);
            continue;
        }

        // Parse hostname from request
        request_buffer[bytes_received] = '\0'; // null terminate the request_buffer to make a valid string
        printf("Request Buffer: \n%s\n-----------------\n", request_buffer);
        char *host_name = extractTargetHost(request_buffer);
        if(!host_name){
            perror("Main func: hostname not found.");
        }else{
            printf("Host: %s\n", host_name);
            free(host_name);
        }

        // Close connection and free resources
        close(client_fd);
    }

    close(proxy_fd);
    return 0;
}