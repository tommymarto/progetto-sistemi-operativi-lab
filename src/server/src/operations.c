#include <operations.h>

#include <mymalloc.h>
#include <configuration.h>
#include <logging.h>

#define logOperationString "operation: %s, state: %s, file: %s"

extern config configs;

fileEntry** openFile(request* r, int* result, int* dim, char* pathname, int flags) {
    boring_file_log(configs.logFileOutput, "fd: %d openFile: %s flags: %d", r->client->clientFd, pathname, flags);

    *dim = 0;

    log_operation(logOperationString, "openFile", "started", pathname);
    fileEntry* expelledFile = filesystem_openFile(result, r, pathname, flags);
    
    // open failed
    if(*result < 0) {
        log_operation(logOperationString, "openFile", "failed", pathname);
        if(expelledFile != NULL) {
            *dim = 1;
        }
    } else {
        log_operation(logOperationString, "openFile", "success", pathname);
        if(expelledFile == NULL) {
            return NULL;
        }
        *dim = 1;
    }

    fileEntry** returnValue = _malloc(sizeof(fileEntry*));
    returnValue[0] = expelledFile;

    return returnValue;
}

fileEntry** readFile(request* r, int* result, int* dim, char* pathname) {
    boring_file_log(configs.logFileOutput, "fd: %d readFile: %s", r->client->clientFd, pathname);
    *dim = 0;

    log_operation(logOperationString, "readFile", "started", pathname);
    fileEntry* fileRead = filesystem_get_fileEntry(pathname);
    
    // read failed
    if(fileRead == NULL) {
        *result = -1;
        log_operation(logOperationString, "readFile", "failed", pathname);
        return NULL;
    }

    *result = 1;
    log_operation(logOperationString, "readFile", "success", pathname);

    fileEntry** returnValue = _malloc(sizeof(fileEntry*));
    returnValue[0] = fileRead;

    *dim = 1;

    return returnValue;
}

fileEntry** readNFiles(request* r, int* result, int* dim, int N) {
    boring_file_log(configs.logFileOutput, "fd: %d readNFiles", r->client->clientFd);
    log_operation(logOperationString, "readNFiles", "started", "");
    *dim = 0;

    fileEntry** filesRead = filesystem_get_n_fileEntry(dim, N);

    if(filesRead == NULL) {
        *result = -1;
        log_operation(logOperationString, "readNFiles", "failed", "");
        return NULL;
    }
    
    *result = 1;
    log_operation(logOperationString, "readNFiles", "success", "");

    return filesRead;
}

fileEntry** writeFile(request* r, int* result, int* dim, fileEntry* file) {
    boring_file_log(configs.logFileOutput, "fd: %d writeFile", r->client->clientFd);
    log_operation(logOperationString, "writeFile", "started", file->pathname);
    fileEntry** filesExpelled = filesystem_writeFile(result, dim, r, file);

    // write failed
    if(*result < 0) {
        log_operation(logOperationString, "writeFile", "failed", file->pathname);
        return NULL;
    }

    log_operation(logOperationString, "writeFile", "success", file->pathname);

    *result = 2;
    return filesExpelled;
}

fileEntry** appendToFile(request* r, int* result, int* dim, fileEntry* file) {
    boring_file_log(configs.logFileOutput, "fd: %d appendToFile: %s", r->client->clientFd, file->pathname);
    log_operation(logOperationString, "appendToFile", "started", file->pathname);
    fileEntry** filesExpelled = filesystem_appendToFile(result, dim, r, file);

    // write failed
    if(*result < 0) {
        log_operation(logOperationString, "appendToFile", "failed", file->pathname);
        return NULL;
    }

    log_operation(logOperationString, "appendToFile", "success", file->pathname);

    *result = 1;
    return filesExpelled;
}

int lockFile(request* r, char* pathname) {
    boring_file_log(configs.logFileOutput, "fd: %d lockFile: %s", r->client->clientFd, pathname);
    log_operation(logOperationString, "lockFile", "started", pathname);
    int result = filesystem_lockAcquire(r, pathname);

    if(result < 0) {
        log_operation(logOperationString, "lockFile", "failed", pathname);
        return result;
    }

    log_operation(logOperationString, "lockFile", "success", pathname);
    return result;
}

int unlockFile(request* r, char* pathname) {
    boring_file_log(configs.logFileOutput, "fd: %d unlockFile: %s", r->client->clientFd, pathname);
    log_operation(logOperationString, "unlockFile", "started", pathname);
    int result = filesystem_lockRelease(r, pathname);

    if(result < 0) {
        log_operation(logOperationString, "unlockFile", "failed", pathname);
        return result;
    }

    log_operation(logOperationString, "unlockFile", "success", pathname);
    return 1;
}

int closeFile(request* r, char* pathname) {
    boring_file_log(configs.logFileOutput, "fd: %d closeFile: %s", r->client->clientFd, pathname);
    log_operation(logOperationString, "closeFile", "started", pathname);
    int descriptor = isFileOpened(r->client, pathname);

    if(descriptor == -1) {
        log_operation(logOperationString, "closeFile", "failed", pathname);
        return -1;
    }
    log_operation(logOperationString, "closeFile", "success", pathname);

    filesystem_lockRelease(r, pathname);
    free(r->client->openedFiles[descriptor]);
    r->client->openedFiles[descriptor] = NULL;
    r->client->canWriteOnFile[descriptor] = false;

    return 1;
}

int removeFile(request* r, char* pathname) {
    boring_file_log(configs.logFileOutput, "fd: %d removeFile: %s", r->client->clientFd, pathname);
    log_operation(logOperationString, "removeFile", "started", pathname);
    int result = filesystem_removeFile(r, pathname);

    if(result == -1) {
        log_operation(logOperationString, "removeFile", "failed", pathname);
        return -1;
    }

    log_operation(logOperationString, "removeFile", "success", pathname);
    return 1;
}