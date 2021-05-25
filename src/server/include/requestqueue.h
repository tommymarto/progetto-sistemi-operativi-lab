#pragma once

#include <request.h>
#include <pthread.h>

#define REQUEST_QUEUE_SIZE 100

typedef struct {
    request* items[REQUEST_QUEUE_SIZE];
    int head;
    int tail;
    bool closed;

    pthread_mutex_t lock;
    pthread_cond_t itemAdded;
    pthread_cond_t itemRemoved;
} request_queue;

void init_request_queue(request_queue* queue);
void insert_request_queue(request_queue* queue, request* item);
request* remove_request_queue(request_queue* queue);
void wait_request_queue_empty(request_queue* queue);
void free_request_queue(request_queue* queue);
void close_request_queue(request_queue* queue);