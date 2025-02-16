#!/bin/bash

NUM_REQUESTS=10  # Set the number of requests

send_request() {
    curl -x http://127.0.0.1:8080 http://www.example.com/
}

# Run multiple requests in parallel
for ((i=1; i<=NUM_REQUESTS; i++)); do
    send_request &
done

# Wait for all background processes to finish
wait

echo "All requests sent!"
