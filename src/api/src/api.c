#include <api.h>

#include <mymalloc.h>
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
static void setLastApiCall(char* opName, char* opStatus, const char* file, int bytesRead, int bytesWritten) {
    lastApiCall.opName = opName;
    lastApiCall.opStatus = opStatus;
    strncpy(lastApiCall.file, file, FILE_LEN);
    lastApiCall.bytesRead = 0;
    lastApiCall.bytesWritten = 0;
    lastApiCall.duration = (t_end.tv_sec - t_start.tv_sec) * 1000000 + t_end.tv_usec - t_start.tv_usec;
}

static void serializeInt(char* dest, int content) {
    dest[3] = (content >> 24) & 0xff;
    dest[2] = (content >> 16) & 0xff;
    dest[1] = (content >> 8) & 0xff;
    dest[0] = content & 0xff;
}

static int deserializeInt(char* src) {
    return (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
}

// returns -1 for errors or the number of bytesWritten
static int writeMessage(char kind, const char* content, int flags) {
    int contentLen = strlen(content);
    // kind is supposed to be a single char
    // size of identifier e.g. 'o' + sizeof content + sizeof flags if present
    size_t payloadLen = sizeof(char) + sizeof(char) * contentLen + sizeof(int);
    size_t headerLen = sizeof(int);
    size_t msgLen = headerLen + payloadLen;
    char* msg = _malloc(msgLen);
    
    // build the message
    serializeInt(msg, (int)payloadLen);
    msg[4] = kind;
    strncpy(msg + 5, content, contentLen);
    serializeInt(msg + msgLen - 4, flags);

    // write message
    int bytesWritten = write(sockfd, msg, msgLen);
    free(msg);
    return bytesWritten;
}

// returns -1 for errors or the number of bytesRead
static int readMessage(char*** content, int** sizes, int* size) {
    int bytesRead, totalBytesRead = 0, numMsg, msgSize;

    // allocate as placeholder
    *size = 1;
    *content = _malloc(sizeof(char*) * 1);
    *sizes = _malloc(sizeof(int) * 1);
    *sizes[0] = -1;

    // read number of messages
    bytesRead = read(sockfd, &numMsg, sizeof(int));
    if(bytesRead == -1) {
        return -1;
    }
    totalBytesRead += bytesRead;

    // realloc for correct size
    *size = numMsg;
    *content = _realloc(*content, sizeof(char*) * numMsg);
    *sizes = _realloc(*sizes, sizeof(int) * numMsg);
    for(int i=0; i<numMsg; i++) {
        (*sizes)[i] = -1;
    }
    

    // read messages
    for(int i=0; i<numMsg; i++) {
        // read size of payload
        bytesRead = read(sockfd, &msgSize, sizeof(int));
        if(bytesRead == -1 || bytesRead != sizeof(int)) {
            return -1;
        }
        totalBytesRead += bytesRead;

        (*sizes)[i] = msgSize;
        
        // read actual payload
        char* msgBuf = _malloc(sizeof(char) * (msgSize + 1));
        bytesRead = read(sockfd, msgBuf, sizeof(char) * msgSize);
        if (bytesRead == -1 || bytesRead != msgSize) {
            return -1;
        }
        totalBytesRead += bytesRead;
        msgBuf[msgSize] = '\0';
        
        (*content)[i] = msgBuf;
    }
    
    return 0;
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
    int bytesWritten = writeMessage('o', pathname, flags);

    // read response
    char** content;
    int* sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim);

    int success = (bytesWritten != -1 && bytesRead != -1) ? 0 : -1;

    // discard content
    for(int i=0; i<dim; i++) {
        if(sizes[i] != -1) {
            free(content[i]);
        }
    }
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

    // write content
    int bytesWritten = writeMessage('r', pathname, 0);

    // read response
    char** content;
    int* sizes, dim;
    int bytesRead = readMessage(&content, &sizes, &dim);

    int success = (bytesWritten != -1 && bytesRead != -1) ? 0 : -1;
    
    // expected a single file to be returned
    if(dim == 1) {
        *buf = content[0];
        *size = sizes[0];
    } else {
        success = -1;

        // discard content
        for(int i=0; i<dim; i++) {
            if(sizes[i] != -1) {
                free(content[i]);
            }
        }
        free(content);
        free(sizes);
    }

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