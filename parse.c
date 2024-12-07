#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llhttp.h>

// Structure to hold the parsed HTTP request
typedef struct {
    char method[10];          // HTTP method (GET, POST, etc.)
    char url[1024];           // URL or path
    char headers[2048];       // Headers as a string
    char body[8192];          // Request body
    int complete;             // Flag to check if parsing is done
} http_request_t;
 
// Helper function to append strings safely
void append_str(char *dest, const char *src, size_t max_len) {
    strncat(dest, src, max_len - strlen(dest) - 1);
}

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
    append_str(req->headers, "\n", sizeof(req->headers));
    snprintf(req->headers + strlen(req->headers), sizeof(req->headers) - strlen(req->headers), "%.*s", (int)length, at);
    append_str(req->headers, ": ", sizeof(req->headers));  // Add a colon to prepare for value
    return 0;
}

int on_header_value(llhttp_t *parser, const char *at, size_t length) {
    http_request_t *req = (http_request_t *)parser->data;
    snprintf(req->headers + strlen(req->headers), sizeof(req->headers) - strlen(req->headers), "%.*s", (int)length, at);
    return 0;
}

int on_body(llhttp_t *parser, const char *at, size_t length) {
    http_request_t *req = (http_request_t *)parser->data;
    snprintf(req->body + strlen(req->body), sizeof(req->body) - strlen(req->body), "%.*s", (int)length, at);
    return 0;
}

int on_message_complete(llhttp_t *parser) {
    http_request_t *req = (http_request_t *)parser->data;
    req->complete = 1;  // Mark parsing as complete
    return 0;
}

// Function to parse the HTTP1 request using llhttp
void parse_http1_request(char *http_request) {
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
    http_request_t req = {0};
    llhttp_init(&parser, HTTP_BOTH, &settings);
    parser.data = &req;

    // Parse the HTTP request
    enum llhttp_errno err = llhttp_execute(&parser, http_request, strlen(http_request));
    if (err == HPE_OK && req.complete) {
        printf("Method: %s\n", req.method);
        printf("URL: %s\n", req.url);
        printf("Headers: %s\n", req.headers);
        if (strlen(req.body) > 0) {
            printf("Body: %s\n", req.body);
        }
    } else {
        fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err), parser.reason);
    }
}


int main(){
    char* sample_req = "GET /pub/WWW/TheProject.html HTTP/1.1\r\nHost: www.w3.org\r\n\r\n";
    parse_http1_request(sample_req);
    return 0;
}