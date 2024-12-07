#include <stdio.h>
#include <sys/socket.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

struct memory {
    char *response;
    size_t size;
};

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp){
    // Get the response already received and stored
    struct memory *mem = (struct memory *)userp;

    // Expand the size of the existing response to append new response chunk
    size_t realsize = size * nmemb;
    char *newresptr = (char *)realloc((void *)mem->response, mem->size + realsize + 1); // 1 extra byte for null termination to sep chunks
    if(newresptr == NULL){
        printf("Memory could not be reallocated to newresptr in write_data().");
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
char *forwardReqToTarget(char *host_name, int target_port, char *request_buffer) {
    CURL *curl =  curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, host_name);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    struct memory chunk;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_buffer);

    CURLcode status = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    return chunk.response;
}
