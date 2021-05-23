#pragma once

#include <stdlib.h>
#include <time.h>

#define FILE_LEN 256

enum OPEN_MODES { O_CREATE = 1, O_LOCK = 1<<1 };

typedef struct {
    char* opName;
    char* opStatus;
    int errorCode;
    char file[FILE_LEN];
    int bytesRead;
    int bytesWritten;
    long duration;
} api_info;

int openConnection(const char* sockname, int msec, const struct timespec abstime);
int closeConnection(const char* sockname);

// request kind: 'o'
int openFile(const char* pathname, int flags);
// request kind: 'r'
int readFile(const char* pathname, void** buf, size_t* size);
// request kind: 'n'
int readNFiles(int N, const char* dirname);
// request kind: 'w'
int writeFile(const char* pathname, const char* dirname);
// request kind: 'a'
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);
// request kind: 'l'
int lockFile(const char* pathname);
// request kind: 'u'
int unlockFile(const char* pathname);
// request kind: 'c'
int closeFile(const char* pathname);
// request kind: 'R'
int removeFile(const char* pathname);