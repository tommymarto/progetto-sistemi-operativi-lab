#pragma once

#include <stdbool.h>

#define MAX_OPENED_FILES 32

typedef struct {
    int clientFd;
    bool isActive;
    // short openedFiles[MAX_OPENED_FILES];
} session;

void init_session(session* s, int clientFd);
void clear_session(session* s);