#pragma once

#include <stdbool.h>

#define MAX_OPENED_FILES 32

typedef struct session session;
struct session {
    int clientFd;
    bool opStatus;
    char* openedFiles[MAX_OPENED_FILES];
    bool canWriteOnFile[MAX_OPENED_FILES];
    char* pathnamePendingLock;

    int (*isFileOpened)(session* self, char* pathname);
};

int isFileOpened(session* self, char* pathname);
void init_session(session* s);
void clear_session(session* s);
int getFreeFileDescriptor(session* s);