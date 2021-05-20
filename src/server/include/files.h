#pragma once

#include <pthread.h>
#include <stdbool.h>

enum OPEN_MODES { O_CREATE = 1, O_LOCK = 1<<1 };

typedef struct {
    char* content;
    char* pathname;
    int length;
    int owner;

    pthread_mutex_t lock;
} fileEntry;

void init_filesystem(int maxFiles);
void free_filesystem();