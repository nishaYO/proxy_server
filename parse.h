#ifndef PARSE_H
#define PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llhttp.h>

// Structure to hold the parsed HTTP request
typedef struct {
    char method[10];          // HTTP method (GET, POST, etc.)
    char url[1024];           // URL or path
    char *headers;    // Headers as a linked list pointer 
    char *body;          // Request body
    int complete;             // Flag to check if parsing is done
} http_request_t;

// Callbacks for the llhttp parser
int on_method(llhttp_t *parser, const char *at, size_t length){
    http_request_t *req = (http_request_t *)parser->data;
    snprintf(req->method + strlen(req->method), sizeof(req->method)- strlen(req->method), "%.*s", (int)length, at);
    return 0;
}

int on_url(llhttp_t *parser, const char *at, size_t length) {
    http_request_t *req = (http_request_t *)parser->data;
    snprintf(req->url + strlen(req->url), sizeof(req->url) - strlen(req->url), "%.*s", (int)length, at);
    return 0;
}

int on_header_field(llhttp_t *parser, const char *at, size_t length) {
    http_request_t *req = (http_request_t *)parser->data;
    char **headers = &(req->headers);

    // Determine the current length of the headers string
    size_t current_length = req->headers ? strlen(req->headers) : 0;

    // Calculate the additional space needed
    size_t additional_length = length + 3; // For ": " and newline
    char *temp_headers = (char *)realloc(req->headers, current_length + additional_length + 1); // +1 for null terminator
    if (!temp_headers) return -1; // Handle memory allocation failure
    *headers = temp_headers;

    // Append the new header field
    snprintf(req->headers + current_length, additional_length + 1, "\n%.*s: ", (int)length, at);

    return 0;
}

int on_header_value(llhttp_t *parser, const char *at, size_t length) {
    http_request_t *req = (http_request_t *)parser->data;
    char **headers = &(req->headers);
    
    // Ensure headers are allocated if not already done
    if (req->headers == NULL) {
        *headers = malloc(1);
        if (!req->headers) return -1;  // Handle memory allocation failure
        req->headers[0] = '\0';  // Initialize the headers string
    }

    // Calculate the new length required for the header value
    size_t new_length = strlen(req->headers) + length + 1;

    // Reallocate memory to fit the new header value
    char *temp_headers = (char*)realloc(req->headers, new_length);
    if (!temp_headers) {
        fprintf(stderr, "Memory allocation failed while resizing headers.\n");
        return -1;
    }

    *headers = temp_headers;

    strncat(req->headers, at, length);
    return 0;
}

int on_body(llhttp_t *parser, const char *at, size_t length) {
    http_request_t *req = (http_request_t *)parser->data;
    char **body = &(req->body); // rvalue returns the address of the body pointer 
    size_t current_length = req->body ? strlen(req->body) : 0;

    // Reallocate memory to hold the new body content
    char *new_body = (char*)realloc(req->body, current_length + length + 1);
    if (!new_body) return -1; // Handle memory allocation failure
    *body = new_body; // Assign temp pointer to the actual body pointer if realloc succeeds

    // Ensure the new body is null-terminated if it was empty
    if (current_length == 0) {
        req->body[0] = '\0'; // Null-terminate to prepare for strncat
    }

    // Append new data
    strncat(req->body, at, length); // adds src in dest right from '/0' char found in src
    return 0;
}

int on_message_complete(llhttp_t *parser) {
    http_request_t *req = (http_request_t *)parser->data;
    req->complete = 1;  // Mark parsing as complete
    return 0;
}

// Function to parse the HTTP1 request using llhttp
http_request_t* parse_http1_request(char *http_request) {
    // Initialize llhttp parser
    llhttp_t parser;
    llhttp_settings_t settings;
    llhttp_settings_init(&settings);

    // Set up callbacks
    settings.on_url = on_url;
    settings.on_header_field = on_header_field;
    settings.on_header_value = on_header_value;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
    settings.on_method = on_method;

    // Prepare a structure to hold parsed data
    http_request_t *req = (http_request_t *)calloc(1, sizeof(http_request_t));
    if (!req) {
        fprintf(stderr, "Memory allocation for http_request_t failed.\n");
        return NULL;
    }

    llhttp_init(&parser, HTTP_BOTH, &settings);
    parser.data = req;

    // Parse the HTTP request
    enum llhttp_errno err = llhttp_execute(&parser, http_request, strlen(http_request));
    if (err == HPE_OK && req->complete) {
        printf("Method: %s\n", req->method);
        printf("URL: %s\n", req->url);
        printf("Headers: %s\n", req->headers);
        
        if (req->body) {
            printf("Body: %s\n", req->body);
        }
    } else {
        fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err), parser.reason);
    }

    return req;
}

#endif 