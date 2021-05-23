#include <worker.h>

#include <logging.h>
#include <mymalloc.h>
#include <operations.h>
#include <comunication.h>
#include <requestqueue.h>
#include <files.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

extern volatile sig_atomic_t threadExit;
extern request_queue requestQueue;

// returns -1 for errors or the number of bytesWritten
int writeMessage(int n, char** content, int* sizes, int fd) {
    // calculate msg length
    size_t headerLen = sizeof(int);
    ssize_t payloadLen = n > 0 ? sizeof(int) : 0;
    for(int i=0; i<n; i++) {
        payloadLen += sizeof(int) + sizeof(char) * (sizes[i] + 1);
    }
    size_t msgLen = headerLen + payloadLen;
    char* msg = _malloc(msgLen);
    
    // response error message trick
    if(n < 0) {
        payloadLen = n;
    }

    // build the message
    char* msgIndex = msg;
    serializeInt(msgIndex, (int)payloadLen);
    msgIndex += sizeof(int);

    if(n > 0) {
        serializeInt(msgIndex, n);
        msgIndex += sizeof(int);
    }
    for(int i=0; i<n; i++) {
        serializeInt(msgIndex, sizes[i]);
        msgIndex += sizeof(int);
        strncpy(msgIndex, content[i], sizeof(char) * sizes[i]);
        msgIndex += sizes[i];
        msgIndex[0] = ' '; // dummy value for easier deserialization
        msgIndex++;
    }

    // write message
    int bytesWritten = writen(fd, msg, msgLen);
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

        fileEntry** files;
        int result;

        switch (r->kind) {
            case 'o': {
                files = openFile(r, &result, r->file->pathname, r->flags);
                break;
            }
            case 'r':
            case 'n':
            case 'w':
            case 'a':
            case 'l':
            case 'u':
            case 'c':
            case 'R':
            default: {
                log_error("unhandled request type '%c', dropping request", r->kind);
                r->free(r);
                continue;
            }
        }

        if(result <= 0) {
            // return error/ping for result
            writeMessage(result, NULL, NULL, r->client->clientFd);
            r->free(r);
            continue;
        }

        char** strings = _malloc(sizeof(char*) * 2 * result);
        int* sizes = _malloc(sizeof(int) * 2 * result);
        for(int i=0; i<result; i+=2) {
            strings[i] = files[i]->pathname;
            sizes[i] = files[i]->pathlen;

            strings[i+1] = files[i]->content;
            sizes[i+1] = files[i]->length;
        }
        
        writeMessage(result, strings, sizes, r->client->clientFd);

        r->free_keep_file(r);
    }

    free(threadId);
    return NULL;
}

void* worker_notify(void* arg) {
    log_info("spawned worker notify");
    while(!threadExit) {
        request* r = filesystem_getClientToNotify();
        if(r == NULL) {
            continue;
        }

        log_info("notifying client");

        bool status = r->client->opStatus;
        if(status) {
            pingOk(r->client->clientFd);
            r->free_keep_file(r);
        } else {
            pingKo(r->client->clientFd);
            r->free(r);
        }

    }

    return NULL;
}