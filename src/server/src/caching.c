#include <caching.h>

#include <configuration.h>
#include <mymalloc.h>
#include <logging.h>
#include <string.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

extern config configs;

fileEntry** cache;
int head, tail;

int currentFileSystemSize = 0;
int currentFileSystemFileCount = 0;
int maxFileSystemSize = 0;
int maxFileSystemFileCount = 0;
int activations = 0;

static void printStats() {
    log_report("╔═══════════════════════════════════════════════╗");
    log_report("║ FILESYSTEM USAGE REPORT:                      ║");
    log_report("║                                               ║");
    log_report("║ current filesystem size     : %10d (MB) ║", currentFileSystemSize / (1024 * 1024));
    log_report("║ current filesystem size     : %10d  (B) ║", currentFileSystemSize);
    log_report("║ current file count          : %10d      ║", currentFileSystemFileCount);
    log_report("║ max filesystem size         : %10d (MB) ║", maxFileSystemSize / (1024 * 1024));
    log_report("║ max filesystem size         : %10d  (B) ║", maxFileSystemSize);
    log_report("║ max file count              : %10d      ║", maxFileSystemFileCount);
    log_report("║                                               ║");
    log_report("║ filesystem size limit       : %10d (MB) ║", configs.maxMemorySize / (1024 * 1024));
    log_report("║ file count limit            : %10d      ║", configs.maxFileCount);
    log_report("║ cache algorithm activations : %10d      ║", activations);
    log_report("╚═══════════════════════════════════════════════╝");
}

void initCachingSystem() {
    cache = _malloc(sizeof(fileEntry*) * configs.maxFileCount);
}

void freeCachingSystem() {
    printStats();
    free(cache);
}

int getCacheBuffer(fileEntry*** cacheBuffer, int* headPtr) {
    *cacheBuffer = cache;
    *headPtr = head;
    return (tail + configs.maxFileCount - head) % configs.maxFileCount;
}

//
// FIFO policy
//

fileEntry* handleInsertionFIFO(fileEntry* file) {
    int index = head;

    int expectedFileSystemFileCount = currentFileSystemFileCount + 1;
    int removedFileSize = 0;
    int itemRemoved = false;

    // file is expected to fit in memory so no queue bad index is accessed, otherwise it's handled in the readMessage phase
    if(expectedFileSystemFileCount > configs.maxFileCount) {
        index = (index + 1) % configs.maxFileCount;
        expectedFileSystemFileCount--;
        removedFileSize = cache[index]->length;
        itemRemoved = true;
    }

    // prepare files to be removed
    fileEntry* fileToRemove = NULL;

    if(itemRemoved) {
        activations++;
        fileToRemove = cache[head];
        head = (head + 1) % configs.maxFileCount;

        log_warn("CACHING SUBSTITUTION ACTIVATED. FILE REMOVED: %s", fileToRemove->pathname);
        boring_file_log(configs.logFileOutput, "cache substitution inFile: %s outFile: %s", file->pathname, fileToRemove->pathname);
    }

    // insert new file
    cache[tail] = file;
    tail = (tail + 1) % configs.maxFileCount;

    // update stats
    currentFileSystemSize -= removedFileSize;
    currentFileSystemFileCount = expectedFileSystemFileCount;
    maxFileSystemFileCount = MAX(maxFileSystemFileCount, currentFileSystemFileCount);

    return fileToRemove;
}

fileEntry** handleEditFIFO(int* dim, fileEntry* file, int newSize) {
    // newSize too big handled by filesystem

    *dim = 0;
    int index = head;
    int indexOfEditedFile = -1;

    int expectedFileSystemSize = currentFileSystemSize + (newSize - file->length);

    // file is expected to fit in memory so no queue bad index is accessed, otherwise it's handled in the readMessage phase
    while(expectedFileSystemSize > configs.maxMemorySize) {
        // skip file to be edited
        if(cache[index] == file) {
            indexOfEditedFile = index;
            index = (index + 1) % configs.maxFileCount;
            continue;
        }

        (*dim)++;
        index = (index + 1) % configs.maxFileCount;
        expectedFileSystemSize -= cache[index]->length;
    }

    // prepare file to remove
    fileEntry** filesToRemove = NULL;

    if(*dim != 0) {
        activations++;
        filesToRemove = _malloc(sizeof(fileEntry*) * (*dim));
        for(int i=0; i<*dim; i++) {
            // don't remove file to be edited
            if(cache[head] == file) {
                i--;
                head = (head + 1) % configs.maxFileCount;
                continue;
            }
            filesToRemove[i] = cache[head];
            head = (head + 1) % configs.maxFileCount;

            log_warn("CACHING SUBSTITUTION ACTIVATED. FILE REMOVED: %s", filesToRemove[i]->pathname);
            boring_file_log(configs.logFileOutput, "cache substitution inFile: %s outFile: %s", file->pathname, filesToRemove[i]->pathname);
        }
    }

    // move edited file if it was skipped
    if(indexOfEditedFile != -1) {
        head = (head + configs.maxFileCount - 1) % configs.maxFileCount;
        cache[head] = cache[indexOfEditedFile];
    }

    // update stats
    currentFileSystemSize = expectedFileSystemSize;
    maxFileSystemSize = MAX(maxFileSystemSize, currentFileSystemSize);

    return filesToRemove;
}

void handleRemovalFIFO(fileEntry* file) {
    int indexOfFileToBeRemoved = head;

    // find file to be removed
    while(true) {
        if(cache[indexOfFileToBeRemoved] == file) {
            indexOfFileToBeRemoved = indexOfFileToBeRemoved;
            break;
        }

        indexOfFileToBeRemoved = (indexOfFileToBeRemoved + 1) % configs.maxFileCount;
    }

    int numOfFilesToBeShifted = (indexOfFileToBeRemoved + configs.maxFileCount - head) % configs.maxFileCount;

    // shift head of buffer
    for(int i=0; i<numOfFilesToBeShifted; i++) {
        indexOfFileToBeRemoved = (indexOfFileToBeRemoved + configs.maxFileCount - 1) % configs.maxFileCount;
        cache[(indexOfFileToBeRemoved + 1) % configs.maxFileCount] = cache[indexOfFileToBeRemoved];
    }
    head = (head + 1) % configs.maxFileCount;

    currentFileSystemSize -= file->length;
    currentFileSystemFileCount--;
}

//
//  LRU policy
//

fileEntry* handleInsertionLRU(fileEntry* file) {
    return NULL;
}

fileEntry** handleEditLRU(int* dim, fileEntry* file, int newSize) {
    return NULL;
}

void handleRemovalLRU(fileEntry* file) {

}

//
//  exposed API
//

fileEntry* handleInsertion(fileEntry* file) {
    fileEntry* result;

    if(strcmp(configs.cachePolicy, "FIFO") == 0) {
        result = handleInsertionFIFO(file);
    } else {
        // if policy not specified, fallback to LRU
        result = handleInsertionLRU(file);
    }
    
    boring_file_log(configs.logFileOutput, "fileInsertion: %s currentFileSystemSize: %d currentFileSystemFileCount: %d", file->pathname, currentFileSystemSize, currentFileSystemFileCount);
    return result;
}

fileEntry** handleEdit(int* dim, fileEntry* file, int newSize) {
    fileEntry** result;

    if(strcmp(configs.cachePolicy, "FIFO") == 0) {
        result = handleEditFIFO(dim, file, newSize);
    } else {
        // if policy not specified, fallback to LRU
        result = handleEditLRU(dim, file, newSize);
    }
    
    boring_file_log(configs.logFileOutput, "fileEdit: %s currentFileSystemSize: %d currentFileSystemFileCount: %d", file->pathname, currentFileSystemSize, currentFileSystemFileCount);
    return result;
}

void handleRemoval(fileEntry* file) {
    if(strcmp(configs.cachePolicy, "FIFO") == 0) {
        handleRemovalFIFO(file);
    } else {
        // if policy not specified, fallback to LRU
        handleRemovalLRU(file);
    }

    boring_file_log(configs.logFileOutput, "fileRemoval: %s currentFileSystemSize: %d currentFileSystemFileCount: %d", file->pathname, currentFileSystemSize, currentFileSystemFileCount);
}
