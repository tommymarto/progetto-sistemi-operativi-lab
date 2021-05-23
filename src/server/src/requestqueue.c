#include <requestqueue.h>

void init_request_queue(request_queue* queue) {
    pthread_mutex_lock(&queue->lock);

    queue->head = 0;
    queue->tail = 0;
    queue->closed = false;
    
    pthread_mutex_unlock(&queue->lock);
}

void insert_request_queue(request_queue* queue, request* item) {
    pthread_mutex_lock(&queue->lock);
    
    // wait for space
    while((queue->tail - queue->head) == REQUEST_QUEUE_SIZE) {
        pthread_cond_wait(&queue->itemRemoved, &queue->lock);
    }

    // insert item
    queue->items[queue->tail % REQUEST_QUEUE_SIZE] = item;
    queue->tail++;
    pthread_cond_signal(&queue->itemAdded);
    
    pthread_mutex_unlock(&queue->lock);
}

request* remove_request_queue(request_queue* queue) {
    pthread_mutex_lock(&queue->lock);
    
    // wait for item
    request* item = NULL;

    if(!queue->closed) {
        while(queue->head == queue->tail && !queue->closed) {
            pthread_cond_wait(&queue->itemAdded, &queue->lock);
        }

        if(!queue->closed) {
            // remove item
            item = queue->items[queue->head % REQUEST_QUEUE_SIZE];
            queue->head++;
            if(queue->head >= REQUEST_QUEUE_SIZE) {
                queue->head -= REQUEST_QUEUE_SIZE;
                queue->tail -= REQUEST_QUEUE_SIZE;
            }
            pthread_cond_signal(&queue->itemRemoved);
        }
    }

    
    pthread_mutex_unlock(&queue->lock);
    return item;
}

void wait_queue_empty(request_queue* queue) {
    pthread_mutex_lock(&queue->lock);
    
    // wait till queue is empty
    while(queue->tail != queue->head) {
        pthread_cond_wait(&queue->itemRemoved, &queue->lock);
    }

    pthread_mutex_unlock(&queue->lock);
}

void free_queue(request_queue* queue) {
    pthread_mutex_lock(&queue->lock);
    
    // clean queue if there are values left
    while(queue->tail != queue->head) {
        request* r = queue->items[queue->head];
        r->free(r);
        queue->head++;
    }

    pthread_mutex_unlock(&queue->lock);
}

void close_request_queue(request_queue* queue) {
    pthread_mutex_lock(&queue->lock);

    // close queue and wake all threads waiting for work
    queue->closed = true;
    pthread_cond_broadcast(&queue->itemAdded);

    pthread_mutex_unlock(&queue->lock);
}