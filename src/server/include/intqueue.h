#pragma once

#include <request.h>
#include <pthread.h>

#define INT_QUEUE_SIZE 500

typedef struct {
    int items[INT_QUEUE_SIZE];
    int head;
    int tail;
    bool closed;

    pthread_mutex_t lock;
    pthread_cond_t itemAdded;
    pthread_cond_t itemRemoved;
} int_queue;

void init_int_queue(int_queue* queue);
void insert_int_queue(int_queue* queue, int item);
int remove_int_queue(int_queue* queue);
void wait_int_queue_empty(int_queue* queue);
void free_int_queue(int_queue* queue);
void close_int_queue(int_queue* queue);