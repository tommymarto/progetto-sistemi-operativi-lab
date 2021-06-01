#include <worker.h>

#include <logging.h>
#include <mymalloc.h>
#include <operations.h>
#include <comunication.h>
#include <requestqueue.h>
#include <configuration.h>
#include <intqueue.h>
#include <files.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

extern bool threadExit();
extern void writeToPipe(int kind, int fd);
extern config configs;
extern int_queue requestQueue;
extern session sessions[];
extern int pipeFds[];


//
//  utils
//

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
        memcpy(msgIndex, content[i], sizeof(char) * sizes[i]);
        msgIndex += sizes[i];
        msgIndex[0] = ' '; // dummy value for easier deserialization
        msgIndex++;
    }

    // write message
    int bytesWritten = writen(fd, msg, msgLen);
    free(msg);
    return bytesWritten;
}

int writeFileArray(int dim, fileEntry* files[], int clientFd) {
    char** strings = _malloc(sizeof(char*) * 2 * dim);
    int* sizes = _malloc(sizeof(int) * 2 * dim);
    for(int i=0; i<dim; i++) {
        int j = 2*i;

        strings[j] = files[i]->pathname;
        sizes[j] = files[i]->pathlen;

        strings[j+1] = files[i]->content;
        sizes[j+1] = files[i]->length;
    }
    
    int result = writeMessage(2 * dim, strings, sizes, clientFd);

    free(strings);
    free(sizes);

    return result;
}

// returns -2 if read less then expected, -1 if errors, 0 if client disconnected or the number of bytes read
int readMessage(int fd, char** msgBuf, int* bufLen) {
    int bytesRead, totalBytesRead = 0;

    *msgBuf = NULL;

    // read size of payload
    char headerBuf[4];
    int msgSize;
    bytesRead = readn(fd, headerBuf, sizeof(int));

    if(bytesRead == -1 || bytesRead == 0) {
        return bytesRead;
    } else if(bytesRead < sizeof(int)) {
        return -2;
    }
    totalBytesRead += sizeof(int);
    msgSize = deserializeInt(headerBuf);
    log_info("found message length: %d", msgSize);

    *bufLen = msgSize;

    // read actual payload
    log_info("reading actual payload...");
    *msgBuf = _malloc(sizeof(char) * (msgSize + 1));
    bytesRead = readn(fd, *msgBuf, sizeof(char) * msgSize);
    if(bytesRead == -1 || bytesRead == 0) {
        return bytesRead;
    } else if(bytesRead < msgSize) {
        return -2;
    }
    totalBytesRead += bytesRead;
    (*msgBuf)[msgSize] = '\0';

    return totalBytesRead;
}

//
//  workers
//

// request handler
int worker_body(request* r, int* bytesWritten) {
    fileEntry** files = NULL;
    int result, dim = 0;

    log_info("received from fd %d file content length: %d", r->client->clientFd, r->file->length);

    switch (r->kind) {
        case 'o': {
            files = openFile(r, &result, &dim, r->file->pathname, r->flags);
            break;
        }
        case 'r': {
            files = readFile(r, &result, &dim, r->file->pathname);
            break;
        }
        case 'n': {
            files = readNFiles(r, &result, &dim, r->flags);
            break;
        }
        case 'w': {
            files = writeFile(r, &result, &dim, r->file);
            break;
        }
        case 'a': {
            files = appendToFile(r, &result, &dim, r->file);
            break;
        }
        case 'l': {
            result = lockFile(r, r->file->pathname);
            break;
        }
        case 'u': {
            result = unlockFile(r, r->file->pathname);
            break;
        }
        case 'c': {
            result = closeFile(r, r->file->pathname);
            break;
        }
        case 'R': {
            result = removeFile(r, r->file->pathname);
            break;
        }
        default: {
            log_error("unhandled request type '%c', dropping request", r->kind);
            result = -1;
        }
    }
    
    // ignore result == 0 because it will be handled by the notifier
    if(result < 0) {
        // return error
        *bytesWritten = pingKo(r->client->clientFd);
    } else if(result > 0) {
        if(dim == 0) {
            *bytesWritten = pingOk(r->client->clientFd);
        } else {
            *bytesWritten = writeFileArray(dim, files, r->client->clientFd);
        }
    }

    if(files != NULL) {
        for(int i=0; i<dim; i++) {
            if(files[i] != NULL) {
                free(files[i]->buf);
            }
            free(files[i]);
        }
    }
    free(files);

    return result;
}

void* worker_start(void* arg) {
    // mask signals so they're handled by the main thread
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, SIGHUP);
    pthread_sigmask(SIG_SETMASK, &sigset, NULL);

    int* threadId = (int*)arg;

    log_info("spawned worker %d", *threadId);
    while(!threadExit()) {

        // pop client from ready queue
        int fd = remove_int_queue(&requestQueue);
        if(fd == -1) {
            continue;
        }

        boring_file_log(configs.logFileOutput, "fd: %d requestServedBy: %d", fd, *threadId);

        // read message from client
        char* msgBuf = NULL;
        int bufLen = 0;
        int bytesRead = readMessage(fd, &msgBuf, &bufLen);
        if(bytesRead == -2) {
            // read less then expected
            log_info("read less then expected bytes, dropping message");
            free(msgBuf);
            continue;
        } if(bytesRead == -1) {
            // read failed
            log_error("failed read from client %d", fd);
            writeToPipe(1, fd);
            free(msgBuf);
            continue;
        } else if (bytesRead == 0) {
            // client has disconnected
            free(msgBuf);
            writeToPipe(0, fd);
            continue;
        }

        boring_file_log(configs.logFileOutput, "fd: %d bytesRead: %d", fd, bytesRead);

        // parse request
        request* r = new_request(msgBuf, bufLen, sessions + fd);

        boring_file_log(configs.logFileOutput, "fd: %d pathname: %s", fd, r->file->pathname);
        log_info("worker %d received message of kind '%c'", *threadId, r->kind);

        int bytesWritten = -2;
        int result = worker_body(r, &bytesWritten);

        if(bytesWritten > 0) {
            boring_file_log(configs.logFileOutput, "fd: %d bytesWritten: %d", fd, bytesWritten);
        }
        
        // write response to client if result != 0 (because if response is 0 client must still wait on a lock)
        if(result != 0) {
            if(result == -1 || result == 1) {
                r->free(r);
            } else if(result == 2) {
                r->free_keep_file_content(r);
            }

            // notify select thread
            writeToPipe(1, fd);
        }

        log_info("worker %d, request completed with status %d", *threadId, result);
    }

    free(threadId);
    return NULL;
}

// notifier
void* worker_notify(void* arg) {
    // mask signals so they're handled by the main thread
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, SIGHUP);
    pthread_sigmask(SIG_SETMASK, &sigset, NULL);

    log_info("spawned worker notify");
    while(!threadExit()) {
        // wait for client to notify
        request* r = filesystem_getClientToNotify();
        if(r == NULL) {
            continue;
        }

        int clientFd = r->client->clientFd;

        boring_file_log(configs.logFileOutput, "fd: %d requestServedByNotifier", clientFd);

        log_info("notifying client %d", clientFd);

        int bytesWritten = 0;
        bool status = r->client->opStatus;
        if(status) {
            bytesWritten = pingOk(clientFd);
            r->free(r);
        } else {            
            bytesWritten = pingKo(clientFd);
            r->free(r);
        }

        // notify select
        log_info("notifying select %d", clientFd);
        writeToPipe(1, clientFd);

        if(bytesWritten > 0) {
            boring_file_log(configs.logFileOutput, "fd: %d bytesWritten: %d", clientFd, bytesWritten);
        }
    }

    return NULL;
}