#include "api.h"

#include <logging.h>

const char* logApi = "operation: %s, state: %s";

int openConnection(const char* sockname, int msec, const struct timespec abstime) {
    log_operation(stdout, logApi, "openConnection", "started");
    log_operation(stdout, logApi, "openConnection", "success");
    return 0;
}

int closeConnection(const char* sockname) {
    log_operation(stdout, logApi, "closeConnection", "started");
    log_operation(stdout, logApi, "closeConnection", "success");
    return 0;
}

int openFile(const char* pathname, int flags) {
    log_operation(stdout, logApi, "openFile", "started");
    log_operation(stdout, logApi, "openFile", "success");
    return 0;
}

int readFile(const char* pathname, void** buf, size_t* size) {
    log_operation(stdout, logApi, "readFile", "started");
    log_operation(stdout, logApi, "readFile", "success");
    return 0;
}

int writeFile(const char* pathname, const char* dirname) {
    log_operation(stdout, logApi, "writeFile", "started");
    log_operation(stdout, logApi, "writeFile", "success");
    return 0;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname) {
    log_operation(stdout, logApi, "appendToFile", "started");
    log_operation(stdout, logApi, "appendToFile", "success");
    return 0;
}

int lockFile(const char* pathname) {
    log_operation(stdout, logApi, "lockFile", "started");
    log_operation(stdout, logApi, "lockFile", "success");
    return 0;
}

int unlockFile(const char* pathname) {
    log_operation(stdout, logApi, "unlockFile", "started");
    log_operation(stdout, logApi, "unlockFile", "success");
    return 0;
}

int closeFile(const char* pathname) {
    log_operation(stdout, logApi, "closeFile", "started");
    log_operation(stdout, logApi, "closeFile", "success");
    return 0;
}

int removeFile(const char* pathname) {
    log_operation(stdout, logApi, "removeFile", "started");
    log_operation(stdout, logApi, "removeFile", "success");
    return 0;
}