#include <api.h>

#include <mymalloc.h>
#include <comunication.h>
#include <fileutils.h>
#include <logging.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
// required nanosleep declaration
int nanosleep(const struct timespec *req, struct timespec *rem);

#define UNIX_PATH_MAX 108

extern int errno;

static int sockfd = -1;

// utils for reporting status
api_info lastApiCall;
static struct timeval t_start, t_end;
static int serverResponseError = 0;
static void setLastApiCall(char* opName, char* opStatus, const char* file, int bytesRead, int bytesWritten) {
    lastApiCall.opName = opName;
    lastApiCall.opStatus = opStatus;
    lastApiCall.errorCode = serverResponseError;
    snprintf(lastApiCall.file, FILE_LEN, "%s", file);
    lastApiCall.bytesRead = bytesRead;
    lastApiCall.bytesWritten = bytesWritten;
    lastApiCall.duration = (t_end.tv_sec - t_start.tv_sec) * 1000000 + t_end.tv_usec - t_start.tv_usec;
}

static bool compareTimes(struct timespec t1, struct timespec t2) {
    if(t1.tv_sec < t2.tv_sec) {
        return true;
    } else if (t1.tv_sec == t2.tv_sec && t1.tv_nsec <= t2.tv_nsec) {
        return true;
    }
    return false;
}

// returns -1 for errors or the number of bytesWritten
static int writeMessage(char kind, const char* pathname, const char* content, int contentLen, int flags) {
    int pathnameLen = strlen(pathname);

    // kind is supposed to be a single char
    // size of identifier e.g. 'o' + sizeof pathname + pathname + sizeof content + content + sizeof flags
    size_t payloadLen = sizeof(char) + sizeof(int) + sizeof(char) * (pathnameLen + 1) + sizeof(int) + sizeof(char) * contentLen + sizeof(int);
    size_t headerLen = sizeof(int);
    size_t msgLen = headerLen + payloadLen;

    char* msg = _malloc(msgLen);
    
    // build the message
    char* msgIndex = msg;

    // write payload len
    serializeInt(msgIndex, (int)payloadLen);
    msgIndex += sizeof(int);

    // write kind
    *msgIndex = kind;
    msgIndex += 1;

    // write pathname and its size
    serializeInt(msgIndex, (int)pathnameLen);
    msgIndex += sizeof(int);
    strncpy(msgIndex, pathname, pathnameLen + 1);
    msgIndex += pathnameLen;
    *msgIndex = ' '; // dummy char for easier deserialization
    msgIndex += 1;
    
    // write content and its size
    serializeInt(msgIndex, (int)contentLen);
    msgIndex += sizeof(int);
    memcpy(msgIndex, content, contentLen);
    msgIndex += contentLen;

    // write flags
    serializeInt(msgIndex, flags);
    msgIndex += sizeof(int);

    // write message
    int bytesWritten = writen(sockfd, msg, msgLen);
    free(msg);

    return bytesWritten > 0 ? bytesWritten : -1;
}

// returns -1 for errors or the number of bytesRead
static int readMessage(char*** content, int** sizes, int* size, char** msg) {
    serverResponseError = 0;
    
    int bytesRead, totalBytesRead = 0, msgSize;
    char intBuf[4];

    // init
    *size = 0;
    *content = NULL;
    *sizes = NULL;
    *msg = NULL;

    // read payload size
    bytesRead = readn(sockfd, intBuf, sizeof(int));
    if(bytesRead != sizeof(int)) {
        return -1;
    }
    msgSize = deserializeInt(intBuf);
    totalBytesRead += bytesRead;

    if(msgSize < 0) {
        serverResponseError = -msgSize;
        return -1;
    } else if (msgSize == 0) {
        // return success, read = headerSize
        return sizeof(int);
    }


    // read payload
    *msg = _malloc(sizeof(char) * (msgSize + 1));
    bytesRead = readn(sockfd, (*msg), sizeof(char) * msgSize);
    if (bytesRead != msgSize) {
        return -1;
    }
    (*msg)[msgSize] = '\0';
    totalBytesRead += bytesRead;

    // deserialize payload
    char* msgIndex = *msg;
    int numMsg, contentSize;

    // deserialize number of messages
    numMsg = deserializeInt(msgIndex);
    msgIndex += sizeof(int);

    // allocate result structures
    *size = numMsg;
    *content = _malloc(sizeof(char*) * numMsg);
    *sizes = _malloc(sizeof(int) * numMsg);
    
    // read messages
    for(int i=0; i<numMsg; i++) {
        
        // deserialize content size
        contentSize = deserializeInt(msgIndex);
        (*sizes)[i] = contentSize;
        msgIndex += sizeof(int);
        
        // save content
        (*content)[i] = msgIndex;
        msgIndex += contentSize;
        msgIndex[0] = '\0';
        msgIndex += 1;
    }
    
    return totalBytesRead;
}

static int simpleApi(const char* pathname, int flags, char* apiName, char apiKind) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    if(pathname == NULL) {
        gettimeofday(&t_end, NULL);
        setLastApiCall(apiName, "failure", "", 0, 0);
        return -1;
    }

    // write content
    int bytesWritten = writeMessage(apiKind, pathname, "", 0, flags);

    // read response
    char **content, *msgBuf;
    int *sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim, &msgBuf);

    int success = (bytesWritten > 0 && bytesRead > 0) ? 0 : -1;

    // discard content
    free(msgBuf);
    free(content);
    free(sizes);

    // handle write result
    gettimeofday(&t_end, NULL);
    setLastApiCall(apiName, success == -1 ? "failure" : "success", pathname, bytesRead, bytesWritten);
    return success;
}

/*
*   APIs
*/

int openConnection(const char* sockname, int msec, const struct timespec abstime) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    if(sockname == NULL) {
        gettimeofday(&t_end, NULL);
        setLastApiCall("openConnection", "failure", "", 0, 0);
        return -1;
    }

    // setup address
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, UNIX_PATH_MAX, "%s", sockname);

    int success;

    // socket creation
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    success = sockfd;

    // try connect if successful till now
    if(success != -1) {
        struct timespec curTime;
        timespec_get(&curTime, TIME_UTC);
        struct timespec sleepTime = { .tv_sec = msec / 1000, .tv_nsec = (msec % 1000) * 1000000};
        
        // repeat until success or timeout
        do {
            // try connect
            success = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
            
            // sleep
            if(success != 0) {
                nanosleep(&sleepTime, NULL);
            }
            
            // update time
            timespec_get(&curTime, TIME_UTC);
        } while(success == -1 && compareTimes(curTime, abstime));
    }

    // success
    gettimeofday(&t_end, NULL);
    setLastApiCall("openConnection", success == -1 ? "failure" : "success", sockname, 0, 0);
    return success;
}

int closeConnection(const char* sockname) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    if(sockname == NULL) {
        gettimeofday(&t_end, NULL);
        setLastApiCall("closeConnection", "failure", "", 0, 0);
        return -1;
    }

    // call to close
    int success = close(sockfd);

    // handle result
    gettimeofday(&t_end, NULL);
    setLastApiCall("closeConnection", success == -1 ? "failure" : "success", sockname, 0, 0);
    return success;
}

int openFile(const char* pathname, int flags) {
    return simpleApi(pathname, flags, "openFile", 'o');
}

// same as openFile with additional support for cache replacement file save
int betterOpenFile(const char* pathname, int flags, const char* dirname) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    if(pathname == NULL) {
        gettimeofday(&t_end, NULL);
        setLastApiCall("betterOpenFile", "failure", "", 0, 0);
        log_report("mhanz");
        return -1;
    }

    // write content
    int bytesWritten = writeMessage('o', pathname, "", 0, flags);

    // read response
    char **content, *msgBuf;
    int *sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim, &msgBuf);

    int success = (bytesWritten > 0 && bytesRead > 0) ? 0 : -1;
    
    if(success == 0) {
        writeResultsToFile(dirname, content, sizes, dim);
    }
    
    free(msgBuf);
    free(content);
    free(sizes);

    // handle write result
    gettimeofday(&t_end, NULL);
    setLastApiCall("betterOpenFile", success == -1 ? "failure" : "success", pathname, bytesRead, bytesWritten);
    return success;
}

int readFile(const char* pathname, void** buf, size_t* size) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    if(pathname == NULL) {
        gettimeofday(&t_end, NULL);
        setLastApiCall("readFile", "failure", "", 0, 0);
        return -1;
    }

    *buf = NULL;
    *size = 0;

    // write content
    int bytesWritten = writeMessage('r', pathname, "", 0, 0);

    // read response
    char **content, *msgBuf;
    int *sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim, &msgBuf);

    int success = (bytesWritten > 0 && bytesRead > 0) ? 0 : -1;

    // expected a single file to be returned
    if(success == 0 && dim == 2) {
        // copy content to user buffer
        *buf = _malloc(sizeof(char) * (sizes[1] + 1));
        memcpy(*buf, content[1], sizes[1]);
        memset((char*)(*buf) + sizes[1], '\0', 1);
        *size = sizes[1];
    } else {
        // set failure
        success = -1;
    }
    
    free(msgBuf);
    free(content);
    free(sizes);

    // handle write result
    gettimeofday(&t_end, NULL);
    setLastApiCall("readFile", success == -1 ? "failure" : "success", pathname, bytesRead, bytesWritten);
    return success;
}

int readNFiles(int N, const char* dirname) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    // write content
    int bytesWritten = writeMessage('n', "", "", 0, N);

    // read response
    char **content, *msgBuf;
    int *sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim, &msgBuf);

    int success = (bytesWritten > 0 && bytesRead > 0) ? 0 : -1;

    if(success == 0) {
        writeResultsToFile(dirname, content, sizes, dim);
    }
    
    free(msgBuf);
    free(content);
    free(sizes);

    // handle write result
    gettimeofday(&t_end, NULL);
    setLastApiCall("readNFiles", success == -1 ? "failure" : "success", "", bytesRead, bytesWritten);
    return success;
}

int writeFile(const char* pathname, const char* dirname) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    char* fileBuf;
    int fileSize = readFromFile(pathname, &fileBuf);
    if(pathname == NULL || fileSize == -1) {
        gettimeofday(&t_end, NULL);
        setLastApiCall("writeFile", "failure", "", 0, 0);
        return -1;
    }

    // write content
    int bytesWritten = writeMessage('w', pathname, fileBuf, fileSize, 0);
    free(fileBuf);

    // read response
    char **content, *msgBuf;
    int *sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim, &msgBuf);

    int success = (bytesWritten > 0 && bytesRead > 0) ? 0 : -1;
    
    if(success == 0) {
        writeResultsToFile(dirname, content, sizes, dim);
    }
    
    free(msgBuf);
    free(content);
    free(sizes);

    // handle write result
    gettimeofday(&t_end, NULL);
    setLastApiCall("writeFile", success == -1 ? "failure" : "success", pathname, bytesRead, bytesWritten);
    return success;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    if(pathname == NULL || buf == NULL) {
        gettimeofday(&t_end, NULL);
        setLastApiCall("appendToFile", "failure", "", 0, 0);
        return -1;
    }

    // write content
    int bytesWritten = writeMessage('a', pathname, buf, size, 0);

    // read response
    char **content, *msgBuf;
    int *sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim, &msgBuf);

    int success = (bytesWritten > 0 && bytesRead > 0) ? 0 : -1;
    
    if(success == 0) {
        writeResultsToFile(dirname, content, sizes, dim);
    }
    
    free(msgBuf);
    free(content);
    free(sizes);

    // handle write result
    gettimeofday(&t_end, NULL);
    setLastApiCall("appendToFile", success == -1 ? "failure" : "success", "", bytesRead, bytesWritten);
    return success;
}

int lockFile(const char* pathname) {
    return simpleApi(pathname, 0, "lockFile", 'l');
}

int unlockFile(const char* pathname) {
    return simpleApi(pathname, 0, "unlockFile", 'u');
}

int closeFile(const char* pathname) {
    return simpleApi(pathname, 0, "closeFile", 'c');
}

int removeFile(const char* pathname) {
    return simpleApi(pathname, 0, "removeFile", 'R');
}