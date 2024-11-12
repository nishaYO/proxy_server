#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

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
        char buffer[1024];
        long buffer_len = sizeof(buffer);

        // Receive data from client socket 
        int bytes_received = recv(client_fd, buffer, buffer_len, 0);
        if(bytes_received < 0){
            perror("RECV");
            close(client_fd);
        }

        buffer[bytes_received] = '\0'; // null terminate the buffer to make a valid string
        printf("Buffer: \n%s\n-----------------\n", buffer);

        // Find host (target server)
        char *host_ptr = strstr(buffer, "Host: ");
        char* host;
        if (host_ptr) {
            host = strtok(host_ptr+6, "\n"); // note: changes original buffer
        }
        printf("Host: %s\n", host);
    }
    close(proxy_fd);
    return 0;
}