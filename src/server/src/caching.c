#include <caching.h>

#include <configuration.h>
#include <mymalloc.h>
#include <logging.h>
#include <string.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

extern config configs;

fileEntry** cache;
int cacheSize;
int head, tail;

int currentFileSystemSize = 0;
int currentFileSystemFileCount = 0;
int maxFileSystemSize = 0;
int maxFileSystemFileCount = 0;
int activations = 0;

static void printActiveFiles() {
    log_fatal("╔═══════════════════════════════════════════════");
    log_fatal("║ Head: %5d Tail: %5d CurrentCount: %5d         ", head, tail, currentFileSystemFileCount);
    log_fatal("║                                               ");
    log_fatal("║ List of files still stored:                   ");

    int curHead = head;

    for(int i=0; i<currentFileSystemFileCount; i++) {
        log_fatal("║ - %s", cache[curHead]->pathname);
        curHead = (curHead + 1) % cacheSize;
    }

    log_fatal("║                                               ");
    log_fatal("╚═══════════════════════════════════════════════");
}

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
    
    boring_file_log(configs.logFileOutput, "maxFileSystemSize: %d", maxFileSystemSize);
    boring_file_log(configs.logFileOutput, "maxFileSystemFileCount: %d", maxFileSystemFileCount);
    boring_file_log(configs.logFileOutput, "cacheAlgorithmActivations: %d", activations);
}

void initCachingSystem() {
    cache = _malloc(sizeof(fileEntry*) * (configs.maxFileCount + 1));
    cacheSize = configs.maxFileCount + 1;
}

void freeCachingSystem() {
    printStats();
    free(cache);
}

int getCacheBuffer(fileEntry*** cacheBuffer, int* headPtr) {
    *cacheBuffer = cache;
    *headPtr = head;
    return currentFileSystemFileCount;
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
        index = (index + 1) % cacheSize;
        expectedFileSystemFileCount--;
        removedFileSize = cache[index]->length;
        itemRemoved = true;
    }

    // prepare files to be removed
    fileEntry* fileToRemove = NULL;

    if(itemRemoved) {
        activations++;
        fileToRemove = cache[head];
        head = (head + 1) % cacheSize;

        log_warn("CACHING SUBSTITUTION ACTIVATED (FILE COUNT). FILE REMOVED: %s FILE INSERTED: %s", fileToRemove->pathname, file->pathname);
        boring_file_log(configs.logFileOutput, "cache substitution (count) inFile: %s outFile: %s", file->pathname, fileToRemove->pathname);
    }

    // insert new file
    cache[tail] = file;
    tail = (tail + 1) % cacheSize;

    // update stats
    currentFileSystemSize -= removedFileSize;
    currentFileSystemFileCount = expectedFileSystemFileCount;
    maxFileSystemFileCount = MAX(maxFileSystemFileCount, currentFileSystemFileCount);

    return fileToRemove;
}

void handleUsageFIFO(fileEntry* file) {

}

fileEntry** handleEditFIFO(int* dim, fileEntry* file, int newSize) {
    // newSize too big handled by filesystem

    *dim = 0;
    int index = head;
    int indexOfEditedFile = -1;

    int expectedFileSystemFileCount = currentFileSystemFileCount;
    int expectedFileSystemSize = currentFileSystemSize + (newSize - file->length);

    // file is expected to fit in memory so no queue bad index is accessed, otherwise it's handled in the readMessage phase
    while(expectedFileSystemSize > configs.maxMemorySize) {
        // skip file to be edited
        if(cache[index] == file) {
            indexOfEditedFile = index;
            index = (index + 1) % cacheSize;
            continue;
        }

        (*dim)++;
        expectedFileSystemSize -= cache[index]->length;
        index = (index + 1) % cacheSize;
        expectedFileSystemFileCount--;
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
                head = (head + 1) % cacheSize;
                continue;
            }
            filesToRemove[i] = cache[head];
            head = (head + 1) % cacheSize;

            log_warn("CACHING SUBSTITUTION ACTIVATED (FILESYSTEM SIZE). FILE REMOVED: %s FILE EDITED: %s", filesToRemove[i]->pathname, file->pathname);
            boring_file_log(configs.logFileOutput, "cache substitution (size) inFile: %s outFile: %s", file->pathname, filesToRemove[i]->pathname);
        }
    }

    // move edited file if it was skipped
    if(indexOfEditedFile != -1) {
        head = (head + cacheSize - 1) % cacheSize;
        cache[head] = cache[indexOfEditedFile];
    }

    // update stats
    currentFileSystemSize = expectedFileSystemSize;
    maxFileSystemSize = MAX(maxFileSystemSize, currentFileSystemSize);
    currentFileSystemFileCount = expectedFileSystemFileCount;
    maxFileSystemFileCount = MAX(maxFileSystemFileCount, currentFileSystemFileCount);

    return filesToRemove;
}

void handleRemovalFIFO(fileEntry* file) {
    int indexOfFileToBeRemoved = head;

    // find file to be removed
    while(true) {
        if(cache[indexOfFileToBeRemoved] == file) {
            break;
        }

        indexOfFileToBeRemoved = (indexOfFileToBeRemoved + 1) % cacheSize;
    }

    int numOfFilesToBeShifted = (indexOfFileToBeRemoved + cacheSize - head + 1) % cacheSize;

    // shift head of buffer
    for(int i=0; i<numOfFilesToBeShifted; i++) {
        indexOfFileToBeRemoved = (indexOfFileToBeRemoved + cacheSize - 1) % cacheSize;
        cache[(indexOfFileToBeRemoved + 1) % cacheSize] = cache[indexOfFileToBeRemoved];
    }
    head = (head + 1) % cacheSize;

    currentFileSystemSize -= file->length;
    currentFileSystemFileCount--;
}

//
//  LRU policy
//

fileEntry* handleInsertionLRU(fileEntry* file) {
    int index = head;

    int expectedFileSystemFileCount = currentFileSystemFileCount + 1;
    int removedFileSize = 0;
    int itemRemoved = false;

    // file is expected to fit in memory so no queue bad index is accessed, otherwise it's handled in the readMessage phase
    if(expectedFileSystemFileCount > configs.maxFileCount) {
        index = (index + 1) % cacheSize;
        expectedFileSystemFileCount--;
        removedFileSize = cache[index]->length;
        itemRemoved = true;
    }

    // prepare files to be removed
    fileEntry* fileToRemove = NULL;

    if(itemRemoved) {
        activations++;
        fileToRemove = cache[head];
        head = (head + 1) % cacheSize;

        log_warn("CACHING SUBSTITUTION ACTIVATED (FILE COUNT). FILE REMOVED: %s FILE INSERTED: %s", fileToRemove->pathname, file->pathname);
        boring_file_log(configs.logFileOutput, "cache substitution (count) inFile: %s outFile: %s", file->pathname, fileToRemove->pathname);
    }

    // insert new file
    cache[tail] = file;
    tail = (tail + 1) % cacheSize;

    // update stats
    currentFileSystemSize -= removedFileSize;
    currentFileSystemFileCount = expectedFileSystemFileCount;
    maxFileSystemFileCount = MAX(maxFileSystemFileCount, currentFileSystemFileCount);

    return fileToRemove;
}

void handleUsageLRU(fileEntry* file) {
    // find used file
    int indexOfUsedFile = head;
    while(true) {
        if(cache[indexOfUsedFile] == file) {
            break;
        }

        indexOfUsedFile = (indexOfUsedFile + 1) % cacheSize;

        if(indexOfUsedFile > 100) {
            exit(EXIT_FAILURE);
        }
    }

    // move file to list tail
    cache[tail] = cache[indexOfUsedFile];
    tail = (tail + 1) % cacheSize;

    // calculate shift count
    int numOfFilesToBeShifted = (indexOfUsedFile + cacheSize - head + 1) % cacheSize;

    // shift head of buffer
    for(int i=0; i<numOfFilesToBeShifted; i++) {
        indexOfUsedFile = (indexOfUsedFile + cacheSize - 1) % cacheSize;
        cache[(indexOfUsedFile + 1) % cacheSize] = cache[indexOfUsedFile];
    }

    head = (head + 1) % cacheSize;
}

fileEntry** handleEditLRU(int* dim, fileEntry* file, int newSize) {
    // newSize too big handled by filesystem

    *dim = 0;
    int index = head;
    int indexOfEditedFile = -1;

    int expectedFileSystemFileCount = currentFileSystemFileCount;
    int expectedFileSystemSize = currentFileSystemSize + (newSize - file->length);

    // file is expected to fit in memory so no queue bad index is accessed, otherwise it's handled in the readMessage phase
    while(expectedFileSystemSize > configs.maxMemorySize) {
        // skip file to be edited
        if(cache[index] == file) {
            indexOfEditedFile = index;
            index = (index + 1) % cacheSize;
            continue;
        }

        (*dim)++;
        expectedFileSystemSize -= cache[index]->length;
        index = (index + 1) % cacheSize;
        expectedFileSystemFileCount--;
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
                head = (head + 1) % cacheSize;
                continue;
            }
            filesToRemove[i] = cache[head];
            head = (head + 1) % cacheSize;

            log_warn("CACHING SUBSTITUTION ACTIVATED (FILESYSTEM SIZE). FILE REMOVED: %s FILE EDITED: %s", filesToRemove[i]->pathname, file->pathname);
            boring_file_log(configs.logFileOutput, "cache substitution (size) inFile: %s outFile: %s", file->pathname, filesToRemove[i]->pathname);
        }
    }

    // move edited file if it was skipped
    if(indexOfEditedFile != -1) {
        cache[tail] = cache[indexOfEditedFile];
        tail = (tail + 1) % cacheSize;
    } else {
        handleUsageLRU(file);
    }

    // update stats
    currentFileSystemSize = expectedFileSystemSize;
    maxFileSystemSize = MAX(maxFileSystemSize, currentFileSystemSize);
    currentFileSystemFileCount = expectedFileSystemFileCount;
    maxFileSystemFileCount = MAX(maxFileSystemFileCount, currentFileSystemFileCount);

    return filesToRemove;
}

void handleRemovalLRU(fileEntry* file) {
    int indexOfFileToBeRemoved = head;

    // find file to be removed
    while(true) {
        if(cache[indexOfFileToBeRemoved] == file) {
            break;
        }

        indexOfFileToBeRemoved = (indexOfFileToBeRemoved + 1) % cacheSize;
    }

    int numOfFilesToBeShifted = (indexOfFileToBeRemoved + cacheSize - head) % cacheSize;

    // shift head of buffer
    for(int i=0; i<numOfFilesToBeShifted; i++) {
        indexOfFileToBeRemoved = (indexOfFileToBeRemoved + cacheSize - 1) % cacheSize;
        cache[(indexOfFileToBeRemoved + 1) % cacheSize] = cache[indexOfFileToBeRemoved];
    }
    head = (head + 1) % cacheSize;

    currentFileSystemSize -= file->length;
    currentFileSystemFileCount--;
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

    // log_error("handleInsertion");
    // printActiveFiles();
    // printStats();
    boring_file_log(configs.logFileOutput, "fileInsertion: %s currentFileSystemSize: %d currentFileSystemFileCount: %d", file->pathname, currentFileSystemSize, currentFileSystemFileCount);
    return result;
}

void handleUsage(fileEntry* file) {
    if(strcmp(configs.cachePolicy, "FIFO") == 0) {
        handleUsageFIFO(file);
    } else {
        // if policy not specified, fallback to LRU
        handleUsageLRU(file);
    }

    // log_error("handleUsage");
    // printActiveFiles();
    // printStats();
}

fileEntry** handleEdit(int* dim, fileEntry* file, int newSize) {
    fileEntry** result;

    if(strcmp(configs.cachePolicy, "FIFO") == 0) {
        result = handleEditFIFO(dim, file, newSize);
    } else {
        // if policy not specified, fallback to LRU
        result = handleEditLRU(dim, file, newSize);
    }
    
    // log_error("handleEdit");
    // printActiveFiles();
    // printStats();
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

    // log_error("handleRemoval");
    // printActiveFiles();
    // printStats();
    boring_file_log(configs.logFileOutput, "fileRemoval: %s currentFileSystemSize: %d currentFileSystemFileCount: %d", file->pathname, currentFileSystemSize, currentFileSystemFileCount);
}
