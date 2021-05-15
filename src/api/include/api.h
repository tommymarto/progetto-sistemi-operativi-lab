#pragma once

#include <stdlib.h>
#include <time.h>

#define FILE_LEN 256
typedef struct {
    char* opName;
    char* opStatus;
    char file[FILE_LEN];
    int bytesRead;
    int bytesWritten;
    long duration;
} api_info;

int openConnection(const char* sockname, int msec, const struct timespec abstime);
int closeConnection(const char* sockname);
int openFile(const char* pathname, int flags);
int readFile(const char* pathname, void** buf, size_t* size);
int readNFiles(int N, const char* dirname);
int writeFile(const char* pathname, const char* dirname);
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);
int lockFile(const char* pathname);
int unlockFile(const char* pathname);
int closeFile(const char* pathname);
int removeFile(const char* pathname);