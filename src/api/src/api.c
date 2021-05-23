#include <api.h>

#include <mymalloc.h>
#include <comunication.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <sys/time.h>

#define UNIX_PATH_MAX 108

extern int errno;

api_info lastApiCall;
static int sockfd = -1;
static struct timeval t_start, t_end;
static int serverResponseError = 0;
static void setLastApiCall(char* opName, char* opStatus, const char* file, int bytesRead, int bytesWritten) {
    lastApiCall.opName = opName;
    lastApiCall.opStatus = opStatus;
    lastApiCall.errorCode = serverResponseError;
    strncpy(lastApiCall.file, file, FILE_LEN);
    lastApiCall.bytesRead = 0;
    lastApiCall.bytesWritten = 0;
    lastApiCall.duration = (t_end.tv_sec - t_start.tv_sec) * 1000000 + t_end.tv_usec - t_start.tv_usec;
}

// returns -1 for errors or the number of bytesWritten
static int writeMessage(char kind, const char* pathname, const char* content, int flags) {
    int pathnameLen = strlen(pathname);
    int contentLen = strlen(content);
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
    strncpy(msgIndex, pathname, pathnameLen);
    msgIndex += pathnameLen;
    *msgIndex = ' '; // dummy char for easier deserialization
    msgIndex += 1;
    
    // write content and its size
    serializeInt(msgIndex, (int)contentLen);
    msgIndex += sizeof(int);
    strncpy(msgIndex, content, contentLen);
    msgIndex += contentLen;

    // write flags
    serializeInt(msgIndex, flags);
    msgIndex += sizeof(int);

    // write message
    int bytesWritten = writen(sockfd, msg, msgLen);
    free(msg);
    return bytesWritten > 0 ? bytesWritten : -1;
}

// returns -error for errors or the number of bytesRead
int readMessage(char*** content, int** sizes, int* size, char** msg) {
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
        return msgSize;
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
        
        // save string
        (*content)[i] = msgIndex;
        msgIndex += contentSize;
        msgIndex[0] = '\0';
        msgIndex += 1;
    }
    
    return totalBytesRead;
}

int openConnection(const char* sockname, int msec, const struct timespec abstime) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    // setup address
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, UNIX_PATH_MAX, "%s", sockname);

    int success;

    // socket creation
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    success = sockfd;
    if (success != -1) {
        // try connect
        success = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    }

    // success
    gettimeofday(&t_end, NULL);
    setLastApiCall("openConnection", success == -1 ? "failure" : "success", sockname, 0, 0);
    return success;
}

int closeConnection(const char* sockname) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    // call to close
    int success = close(sockfd);

    // handle result
    gettimeofday(&t_end, NULL);
    setLastApiCall("closeConnection", success == -1 ? "failure" : "success", sockname, 0, 0);
    return 0;
}

int openFile(const char* pathname, int flags) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    // write content
    int bytesWritten = writeMessage('o', pathname, "", flags);

    // read response
    char **content, *msgBuf;
    int *sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim, &msgBuf);

    int success = (bytesWritten != -1 && bytesRead != -1) ? 0 : -1;

    // discard content
    free(msgBuf);
    free(content);
    free(sizes);

    // handle write result
    gettimeofday(&t_end, NULL);
    setLastApiCall("openFile", success == -1 ? "failure" : "success", pathname, bytesRead, bytesWritten);
    return success;
}

int readFile(const char* pathname, void** buf, size_t* size) {
    errno = 0;
    gettimeofday(&t_start, NULL);

    *buf = NULL;
    *size = 0;

    // write content
    int bytesWritten = writeMessage('r', pathname, "", 0);

    // read response
    char **content, *msgBuf;
    int *sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim, &msgBuf);

    int success = (bytesWritten != -1 && bytesRead != -1) ? 0 : -1;
    
    // expected a single file to be returned
    if(success && dim == 1) {
        // copy content to user buffer
        *buf = _malloc(sizeof(char) * (sizes[0] + 1));
        strncpy(*buf, content[0], sizes[0]);
        memset((char*)(*buf) + sizes[0], '\0', 1);
        *size = sizes[0];
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
    return bytesWritten == -1 ? -1 : 0;
}

int readNFiles(int N, const char* dirname) {
    errno = 0;

    return 0;
}

int writeFile(const char* pathname, const char* dirname) {
    errno = 0;
    // // log_operation(logApi, "writeFile", "started");
    // // log_operation(logApi, "writeFile", "success");
    return 0;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname) {
    errno = 0;
    // // log_operation(logApi, "appendToFile", "started");
    // // log_operation(logApi, "appendToFile", "success");
    return 0;
}

int lockFile(const char* pathname) {
    errno = 0;
    // // log_operation(logApi, "lockFile", "started");
    // // log_operation(logApi, "lockFile", "success");
    return 0;
}

int unlockFile(const char* pathname) {
    errno = 0;
    // // log_operation(logApi, "unlockFile", "started");
    // // log_operation(logApi, "unlockFile", "success");
    return 0;
}

int closeFile(const char* pathname) {
    errno = 0;
    // // log_operation(logApi, "closeFile", "started");
    // // log_operation(logApi, "closeFile", "success");
    return 0;
}

int removeFile(const char* pathname) {
    errno = 0;
    // // log_operation(logApi, "removeFile", "started");
    // // log_operation(logApi, "removeFile", "success");
    return 0;
}