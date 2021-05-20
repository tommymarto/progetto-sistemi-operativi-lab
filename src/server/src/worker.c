#include <worker.h>

#include <logging.h>
#include <mymalloc.h>
#include <requestqueue.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

extern volatile sig_atomic_t threadExit;
extern request_queue requestQueue;

// returns -1 for errors or the number of bytesWritten
static bool writeMessage(int n, char** content, int* sizes, int fd) {
    // calculate msg length
    size_t headerLen = sizeof(int);
    size_t payloadLen = 0;
    for(int i=0; i<n; i++) {
        payloadLen += sizeof(int) + sizeof(char) * sizes[i];
    }
    size_t msgLen = headerLen + payloadLen;
    char* msg = _malloc(msgLen);
    
    // build the message
    int index = 0;
    serializeInt(msg, n);
    index += sizeof(int);
    for(int i=0; i<n; i++) {
        serializeInt(msg + index, sizes[i]);
        index += sizeof(int);
        strncpy(msg + index, content[i], sizeof(char) * sizes[i]);
        index += sizes[i];
    }

    // write message
    int bytesWritten = write(fd, msg, msgLen);
    free(msg);
    return bytesWritten;
}

void* worker_start(void* arg) {
    int* threadId = (int*)arg;

    log_info("spawned worker");
    while(!threadExit) {
        request* r = remove_request_queue(&requestQueue);
        if(r == NULL) {
            continue;
        }
        log_info("request '%c', served by worker %d", r->kind, *threadId);

        char* strings[3];
        strings[0] = "sas";
        strings[1] = "sutus";
        strings[2] = "ole una frase un pochino piu lunga perche si mi va cosi";
        int sizes[3];
        sizes[0] = strlen(strings[0]);
        sizes[1] = strlen(strings[1]);
        sizes[2] = strlen(strings[2]);
        
        writeMessage(3, strings, sizes, r->client->clientFd);

        r->free(r);
    }

    free(threadId);
    return NULL;
}