#include <intqueue.h>

#include <logging.h>

void init_int_queue(int_queue* queue) {
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->itemAdded, NULL);
    pthread_cond_init(&queue->itemRemoved, NULL);

    queue->head = 0;
    queue->tail = 0;
    queue->closed = false;
}

void insert_int_queue(int_queue* queue, int item) {
    pthread_mutex_lock(&queue->lock);
    
    // wait for space
    while((queue->tail - queue->head) == INT_QUEUE_SIZE) {
        pthread_cond_wait(&queue->itemRemoved, &queue->lock);
    }

    // insert item
    queue->items[queue->tail % INT_QUEUE_SIZE] = item;
    queue->tail++;
    pthread_cond_signal(&queue->itemAdded);
    
    pthread_mutex_unlock(&queue->lock);
}

int remove_int_queue(int_queue* queue) {
    pthread_mutex_lock(&queue->lock);
    
    // wait for item
    int item = -1;

    if(!queue->closed) {
        while(queue->head == queue->tail && !queue->closed) {
            pthread_cond_wait(&queue->itemAdded, &queue->lock);
        }

        if(!queue->closed) {
            // remove item
            item = queue->items[queue->head % INT_QUEUE_SIZE];
            queue->head++;
            if(queue->head >= INT_QUEUE_SIZE) {
                queue->head -= INT_QUEUE_SIZE;
                queue->tail -= INT_QUEUE_SIZE;
            }
            pthread_cond_signal(&queue->itemRemoved);
        }
    }

    
    pthread_mutex_unlock(&queue->lock);
    return item;
}

void wait_int_queue_empty(int_queue* queue) {
    pthread_mutex_lock(&queue->lock);
    
    // wait till queue is empty
    while(queue->tail != queue->head) {
        pthread_cond_wait(&queue->itemRemoved, &queue->lock);
    }

    pthread_mutex_unlock(&queue->lock);
}

void free_int_queue(int_queue* queue) {
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->itemAdded);
    pthread_cond_destroy(&queue->itemRemoved);
}

void close_int_queue(int_queue* queue) {
    pthread_mutex_lock(&queue->lock);

    // close queue and wake all threads waiting for work
    queue->closed = true;
    pthread_cond_broadcast(&queue->itemAdded);

    pthread_mutex_unlock(&queue->lock);
}