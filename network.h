#ifndef NETWORK_H
#define NETWORK_H

#include "parse.h" // To use http_request_t
#include <stdio.h>
#include <sys/socket.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

struct memory {
    char *response;
    size_t size;
};

// Convert headers from string to linked list 
typedef struct curl_slist curl_slist;

void convert_to_linked_list(char* headers, curl_slist **headersll){
    char *headers_copy = strdup(headers); // Create a duplicate of headers
    if (!headers_copy) {
        fprintf(stderr, "Failed to duplicate headers.\n");
        return;
    }

    char *token = strtok(headers_copy, "\n");  // Split the header by \n
    while (token != NULL) {
        *headersll = curl_slist_append(*headersll, token); // Append each header to a new node
        token = strtok(NULL, "\n");
    }
    free(headers_copy);
};
// Assemble the response from the target server
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp){
    // Get the response already received and stored
    struct memory *mem = (struct memory *)userp;

    // Expand the size of the existing response to append new response chunk
    size_t realsize = size * nmemb;
    char *newresptr = (char *)realloc(mem->response, mem->size + realsize + 1); // 1 extra byte for null termination to sep chunks
    if(!newresptr){
        printf("Memory could not be reallocated to newresptr in write_data() for %s.\n", (char*)buffer);
        return 0;
    }

    // Append the buffer to the new memory space 
    memcpy(newresptr + mem->size, buffer, realsize);

    // Update the mem struct 
    mem->response = newresptr;
    mem->size +=realsize;

    // Null terminate the response 
    mem->response[mem->size] = '\0';

    // Return bytesize succesfully handled in this callback
    return realsize;
}

// Function to forward a request to a target host and return the response
char *forward_request_to_target(http_request_t *request) {
    // Initialize Curl
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize cURL\n");
        return NULL;
    }

    // Set request URL 
    curl_easy_setopt(curl, CURLOPT_URL, request->url);
    
    // Allocate memory for response headers and body
    struct memory headers_chunk = { .response = malloc(1), .size = 0 };
    struct memory body_chunk = { .response = malloc(1), .size = 0 };

    if (!headers_chunk.response || !body_chunk.response) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(headers_chunk.response);
        free(body_chunk.response);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Assemble response headers and body using callbacks
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&headers_chunk);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body_chunk);

    // Set method for request
    if (strcmp(request->method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (strcmp(request->method, "GET") == 0) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request->method);
    }

    // Convert req headers from string to linked list 
    curl_slist *headersll = NULL;
    if (request->headers && strlen(request->headers) != 0) {
        convert_to_linked_list(request->headers, &headersll);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headersll);
    }

    // Set request body 
    if (request->body) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
    }

    // Make http request with curl
    CURLcode status = curl_easy_perform(curl);
    // Check if request successful
    if (status != CURLE_OK) {
        fprintf(stderr, "cURL error: %s\n", curl_easy_strerror(status));
        free(headers_chunk.response);
        free(body_chunk.response);
        if (headersll) curl_slist_free_all(headersll);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Assemble and create final http response 
    size_t total_size = headers_chunk.size + body_chunk.size + 50;
    char *final_response = malloc(total_size);
    if (!final_response) {
        fprintf(stderr, "Memory allocation failed for final response.\n");
        free(headers_chunk.response);
        free(body_chunk.response);
        if (headersll) curl_slist_free_all(headersll);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // append headers and body to final response
    snprintf(final_response, total_size, "%s\r\n%s\r\n", headers_chunk.response, body_chunk.response);

    // Cleanup
    free(headers_chunk.response);
    free(body_chunk.response);
    if (headersll) curl_slist_free_all(headersll);
    curl_easy_cleanup(curl);

    return final_response;
}

// Send the http response from target server back to the client socket
int send_response_to_client(int client_fd, char *response) {
    size_t len = strlen(response);
    size_t total_sent = 0;
    // Loop to avoid data truncation
    while (total_sent < len) {
        ssize_t sent_bytes = send(client_fd, response + total_sent, len - total_sent, 0);
        if (sent_bytes < 0) {
            perror("SEND");
            return -1;
        }
        total_sent += sent_bytes;
    }
    return 0;
}

#endif
