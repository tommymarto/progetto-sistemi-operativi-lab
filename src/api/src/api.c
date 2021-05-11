#include "api.h"

#include <logging.h>

const char* logApi = "operation: %s, state: %s";


int openConnection(const char* sockname, int msec, const struct timespec abstime) {
    log_info_stdout(logApi, "openConnection", "started");
    log_info_stdout(logApi, "openConnection", "success");
    return 0;
}

int closeConnection(const char* sockname) {
    log_info_stdout(logApi, "closeConnection", "started");
    log_info_stdout(logApi, "closeConnection", "success");
    return 0;
}

int openFile(const char* pathname, int flags) {
    log_info_stdout(logApi, "openFile", "started");
    log_info_stdout(logApi, "openFile", "success");
    return 0;
}

int readFile(const char* pathname, void** buf, size_t* size) {
    log_info_stdout(logApi, "readFile", "started");
    log_info_stdout(logApi, "readFile", "success");
    return 0;
}

int writeFile(const char* pathname, const char* dirname) {
    log_info_stdout(logApi, "writeFile", "started");
    log_info_stdout(logApi, "writeFile", "success");
    return 0;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname) {
    log_info_stdout(logApi, "appendToFile", "started");
    log_info_stdout(logApi, "appendToFile", "success");
    return 0;
}

int lockFile(const char* pathname) {
    log_info_stdout(logApi, "lockFile", "started");
    log_info_stdout(logApi, "lockFile", "success");
    return 0;
}

int unlockFile(const char* pathname) {
    log_info_stdout(logApi, "unlockFile", "started");
    log_info_stdout(logApi, "unlockFile", "success");
    return 0;
}

int closeFile(const char* pathname) {
    log_info_stdout(logApi, "closeFile", "started");
    log_info_stdout(logApi, "closeFile", "success");
    return 0;
}

int removeFile(const char* pathname) {
    log_info_stdout(logApi, "removeFile", "started");
    log_info_stdout(logApi, "removeFile", "success");
    return 0;
}