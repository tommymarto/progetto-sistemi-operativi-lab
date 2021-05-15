#include <api.h>

#include <stdio.h>
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

int openConnection(const char* sockname, int msec, const struct timespec abstime) {
    errno = 0;

    gettimeofday(&t_start, NULL);

    // socket creation
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        gettimeofday(&t_end, NULL);
        setLastApiCall("openConnection", "failed", sockname, 0, 0);
        return -1;
    }

    // setup address
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, UNIX_PATH_MAX, "%s", sockname);

    // try connect
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        gettimeofday(&t_end, NULL);
        setLastApiCall("openConnection", "failed", sockname, 0, 0);
        return -1;
    }

    // success
    gettimeofday(&t_end, NULL);
    setLastApiCall("openConnection", "success", sockname, 0, 0);
    return 0;
}

int closeConnection(const char* sockname) {
    errno = 0;

    gettimeofday(&t_start, NULL);

    // handle close failure
    if(close(sockfd) == -1) {
        gettimeofday(&t_end, NULL);
        setLastApiCall("closeConnection", "failed", sockname, 0, 0);
        return -1;
    }

    // success
    gettimeofday(&t_end, NULL);
    setLastApiCall("closeConnection", "success", sockname, 0, 0);
    return 0;
}

int openFile(const char* pathname, int flags) {
    errno = 0;
    // // log_operation(logApi, "openFile", "started");

    // log_message("hellew from client");
    write(sockfd, "hellew from client", 18);

    // // log_operation(logApi, "openFile", "success");
    return 0;
}

int readFile(const char* pathname, void** buf, size_t* size) {
    errno = 0;
    // // log_operation(logApi, "readFile", "started");
    // // log_operation(logApi, "readFile", "success");
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