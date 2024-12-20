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

void convertToLinkedList(char* headers, curl_slist **headersll){
    char *token = strtok(headers, "\n");  // Split the header by \n
    // Loop through all tokens(key-value pairs)
    while (token != NULL) {
        *headersll = curl_slist_append(*headersll, token); // append each header to a new node
        token = strtok(NULL, "\n");
    }
};

// Assemble the response from the target server
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp){
    // Get the response already received and stored
    struct memory *mem = (struct memory *)userp;

    // Expand the size of the existing response to append new response chunk
    size_t realsize = size * nmemb;
    char *newresptr = (char *)realloc(mem->response, mem->size + realsize + 1); // 1 extra byte for null termination to sep chunks
    if(!newresptr){
        printf("Memory could not be reallocated to newresptr in write_data().");
        free(mem->response);
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
char *forwardReqToTarget(http_request_t *request) {
    CURL *curl =  curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize cURL\n");
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, request->url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    struct memory chunk;
    chunk.response = malloc(1);  // Allocate memory for the initial empty response
    chunk.size = 0;              // Initialize size to 0
    if (!chunk.response) {
        fprintf(stderr, "Memory allocation failed for response buffer.\n");
        curl_easy_cleanup(curl);
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
    // Set method dynamically
    if (strcmp(request->method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    }else if (strcmp(request->method, "GET") == 0) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request->method); //  for put, delete, patch, head, options etc
    };

    curl_slist *headersll = NULL; 
    if(request->headers && strlen(request->headers)!=0) {
        convertToLinkedList(request->headers, &headersll);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headersll);
    }

    if(request->body){
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
    }

    CURLcode status = curl_easy_perform(curl);
    if(status != CURLE_OK) {
        fprintf(stderr, "cURL error: %s\n", curl_easy_strerror(status));
        free(chunk.response);
        if (headersll) curl_slist_free_all(headersll);
        curl_easy_cleanup(curl);
        return NULL;
    }

    curl_easy_cleanup(curl);
    if (headersll) curl_slist_free_all(headersll);
    return chunk.response;
}

#endif 