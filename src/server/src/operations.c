#include <operations.h>

#include <mymalloc.h>
#include <logging.h>

#define logOperationString "operation: %s, state: %s, file: %s"

fileEntry** openFile(request* r, int* result, char* pathname, int flags) {
    log_operation(logOperationString, "openFile", "started", pathname);
    fileEntry* expelledFile = filesystem_openFile(result, r, pathname, flags);
    
    // open failed
    if(*result < 0) {
        log_operation(logOperationString, "openFile", "failed", pathname);
        return NULL;
    }

    log_operation(logOperationString, "openFile", "success", pathname);
    
    // format output
    if(*result == 0) {
        return NULL;
    }

    fileEntry** returnValue = _malloc(sizeof(fileEntry*));
    returnValue[0] = expelledFile;

    return returnValue;
}

fileEntry** readFile(request* r, int* result, char* pathname) {
    return NULL;
}

fileEntry** readNFiles(request* r, int* result, int N) {
    return NULL;
}

fileEntry** writeFile(request* r, int* result, char* pathname, char* content, int size) {
    return NULL;
}

fileEntry** appendToFile(request* r, int* result, char* pathname, char* content, int size) {
    return NULL;
}

int lockFile(request* r, char* pathname) {
    log_operation(logOperationString, "openFile", "started", pathname);
    int result = filesystem_lockAcquire(r, pathname);

    if(result < 0) {
        log_operation(logOperationString, "openFile", "failed", pathname);
        return result;
    }

    log_operation(logOperationString, "openFile", "success", pathname);
    return 0;
}

int unlockFile(request* r, char* pathname) {
    return 0;
}

int closeFile(request* r, char* pathname) {
    return 0;
}

int removeFile(request* r, char* pathname) {
    return 0;
}