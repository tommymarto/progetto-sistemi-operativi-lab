#pragma once

#define _GNU_SOURCE

#include <pthread.h>
#include <stdbool.h>
#include <request.h>

enum OPEN_MODES { O_CREATE = 1, O_LOCK = 1<<1 };

// forward declaration
typedef struct request request;
struct request;

typedef struct fileEntry fileEntry;
struct fileEntry {
    char* buf;
    char* content;
    char* pathname;
    int bufLen;
    int length;
    int pathlen;
    int owner;
};

fileEntry* deep_copy_file(fileEntry* r);

void free_filesystem();
void init_filesystem();
void close_filesystem_pending_locks();
request* filesystem_getClientToNotify();

bool filesystem_fileExists(char* pathname);

fileEntry* filesystem_openFile(int* result, request* r, char* pathname, int flags);
fileEntry* filesystem_get_fileEntry(char* pathname);
fileEntry** filesystem_writeFile(int* result, int* dim, request* r, fileEntry* f);
int filesystem_lockAcquire(request* r, char* pathname);