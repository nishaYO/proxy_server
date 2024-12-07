#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>

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

        if(host_name == NULL){
            perror("Main func: hostname not found.");
            close(client_fd);
            continue;
        }

        printf("Host: %s\n", host_name);

        // Find target server port number from request buffer
        int *target_port = getHostPortNumber(host_name); // todo: pass host_name by value 

        // Forward req to the target server and get response 
        char *response = forwardRequestToTarget(host_name, target_port, request_buffer);
        if (response == NULL){
            printf("Error while getting response from forwardRequestToTarget() in main ().");
        }

        // Send the response to the client 
        int *status = sendResponseToClient(client_fd, response);
        if (status!=0){
            printf("Unsuccessful status from sendResponseToClient");
        }

        // Close connection and free resources
        free(host_name);
        close(client_fd);
    }

    close(proxy_fd);
    return 0;
}