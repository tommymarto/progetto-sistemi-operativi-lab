#pragma once

#include <configuration.h>
#include <sys/socket.h>     // has SOMAXCONN
#include <sys/select.h>     // has FD_SETSIZE

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define DEFAULT_CONFIGURATION_FILE "config.txt"
#define DEFAULT_SOCKET_FILENAME "fileStorageSocket.sk"
#define DEFAULT_CACHE_POLICY "FIFO"
#define DEFAULT_N_THREAD_WORKERS 4
#define DEFAULT_MAX_MEMORY_SIZE_MB 128
#define DEFAULT_MAX_FILE_COUNT 1000
#define DEFAULT_MAX_FILE_SIZE 1 << 20
#define DEFAULT_MAX_CLIENTS MIN(SOMAXCONN, FD_SETSIZE)
#define DEFAULT_LOG_VERBOSITY 2

void handleConfiguration(int argc, char* argv[]);