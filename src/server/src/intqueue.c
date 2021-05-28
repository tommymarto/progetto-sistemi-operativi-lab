#include <intqueue.h>

#include <mylocks.h>

void init_int_queue(int_queue* queue) {
    _pthread_mutex_init(&queue->lock);
    _pthread_cond_init(&queue->itemAdded);
    _pthread_cond_init(&queue->itemRemoved);

    queue->head = 0;
    queue->tail = 0;
    queue->closed = false;
}

void insert_int_queue(int_queue* queue, int item) {
    _pthread_mutex_lock(&queue->lock);
    
    // wait for space
    while((queue->tail - queue->head) == INT_QUEUE_SIZE) {
        _pthread_cond_wait(&queue->itemRemoved, &queue->lock);
    }

    // insert item
    queue->items[queue->tail % INT_QUEUE_SIZE] = item;
    queue->tail++;
    _pthread_cond_signal(&queue->itemAdded);
    
    _pthread_mutex_unlock(&queue->lock);
}

int remove_int_queue(int_queue* queue) {
    _pthread_mutex_lock(&queue->lock);
    
    // wait for item
    int item = -1;

    if(!queue->closed) {
        while(queue->head == queue->tail && !queue->closed) {
            _pthread_cond_wait(&queue->itemAdded, &queue->lock);
        }

        if(!queue->closed) {
            // remove item
            item = queue->items[queue->head % INT_QUEUE_SIZE];
            queue->head++;
            if(queue->head >= INT_QUEUE_SIZE) {
                queue->head -= INT_QUEUE_SIZE;
                queue->tail -= INT_QUEUE_SIZE;
            }
            _pthread_cond_signal(&queue->itemRemoved);
        }
    }

    
    _pthread_mutex_unlock(&queue->lock);
    return item;
}

void wait_int_queue_empty(int_queue* queue) {
    _pthread_mutex_lock(&queue->lock);
    
    // wait till queue is empty
    while(queue->tail != queue->head) {
        _pthread_cond_wait(&queue->itemRemoved, &queue->lock);
    }

    _pthread_mutex_unlock(&queue->lock);
}

void free_int_queue(int_queue* queue) {
    _pthread_mutex_destroy(&queue->lock);
    _pthread_cond_destroy(&queue->itemAdded);
    _pthread_cond_destroy(&queue->itemRemoved);
}

void close_int_queue(int_queue* queue) {
    _pthread_mutex_lock(&queue->lock);

    // close queue and wake all threads waiting for work
    queue->closed = true;
    _pthread_cond_broadcast(&queue->itemAdded);

    _pthread_mutex_unlock(&queue->lock);
}