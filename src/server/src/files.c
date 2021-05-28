#include <files.h>

#include <mymalloc.h>
#include <mylocks.h>
#include <caching.h>
#include <logging.h>
#include <configuration.h>
#include <requestqueue.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#define HASHTABLE_SIZE 4096
#define MAX_CLIENTS_WAITING_FOR_LOCK 32

extern config configs;

typedef struct hashEntry hashEntry;
struct hashEntry {
    fileEntry* file;
    hashEntry* next;

    request* waitingQueue[MAX_CLIENTS_WAITING_FOR_LOCK];
    int queueHead;
    int queueTail;
};

pthread_rwlock_t tableLock = PTHREAD_RWLOCK_INITIALIZER;
hashEntry* filesystem[HASHTABLE_SIZE] = { NULL };

request_queue clientsToNotify;

/* 
    internal functions
*/

unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

static void printStats() {
    log_report("╔═══════════════════════════════════════════════");
    log_report("║ FILESYSTEM USAGE ADDITIONAL INFO:             ");
    log_report("║                                               ");
    log_report("║ List of files still stored:                   ");

    for(int i=0; i<HASHTABLE_SIZE; i++) {
        hashEntry* e = filesystem[i];
        while(e != NULL) {
            log_report("║ - %s %d", e->file->pathname, i);
            e = e->next;
        }
    }

    log_report("║                                               ");
    log_report("╚═══════════════════════════════════════════════");
}

fileEntry* deep_copy_file(fileEntry* f) {
    fileEntry* result = _malloc(sizeof(fileEntry));
    memcpy(result, f, sizeof(fileEntry));

    // deep copy file
    result->buf = _malloc(f->bufLen);
    memcpy(result->buf, f->buf, f->bufLen);
    
    // adjust pointers
    result->content = result->buf + (f->content - f->buf);
    result->pathname = result->buf + (f->pathname - f->buf);
    return result;
}

void free_hashEntry(hashEntry* e) {
    free(e->file->buf);
    free(e->file);
    free(e);
}

void free_hashEntry_keep_file(hashEntry* e) {
    free(e);
}

// must be called only when the appropriate lock is acquired
static hashEntry* remove_hashEntry_ref(char* pathname, int hashIndex, bool keepFile) {
    // search for entry in the table
    hashEntry* current = filesystem[hashIndex];
    hashEntry* prev = NULL;
    while(current != NULL && strcmp(current->file->pathname, pathname) != 0) {
        prev = current;
        current = current->next;
    }

    if(current != NULL) {
        if(prev == NULL) {
            filesystem[hashIndex] = current->next;
        } else {
            prev->next = current->next;
        }

        if(keepFile) {
            free_hashEntry_keep_file(current);
        } else {
            free_hashEntry(current);
        }
    }

    return current;
}

// must be called only when the appropriate lock is acquired
static hashEntry* get_hashEntry_ref(char* pathname, int hashIndex) {
    // search for entry in the table
    hashEntry* current = filesystem[hashIndex];
    while(current != NULL && strcmp(current->file->pathname, pathname) != 0) {
        current = current->next;
    }

    return current;
}

// must be called only when the appropriate lock is acquired
static fileEntry* get_fileEntry_ref(char* pathname, int hashIndex) {
    hashEntry* current = get_hashEntry_ref(pathname, hashIndex);

    if(current != NULL) {
        return current->file;
    }
    return NULL;
}

bool put_fileEntry_ref(fileEntry* newFile, int index) {
    // make sure owner is correctly set
    newFile->owner = -1;

    hashEntry* newHash = _malloc(sizeof(hashEntry));
    newHash->file = newFile;
    newHash->queueHead = 0;
    newHash->queueTail = 0;

    // put entry in the table if it does not esists
    if(get_fileEntry_ref(newFile->pathname, index) == NULL) {
        newHash->next = filesystem[index];
        filesystem[index] = newHash;
        return true;
    }

    free(newHash);
    return false;
}

// returns 1 if lock is immediately acquired, 0 if request is in queue, -1 if request failed
static int lockfile(request* r, char* pathname, int index, int sessionFileIndex) {
    hashEntry* h = get_hashEntry_ref(pathname, index);

    session* client = r->client;

    if(h != NULL) {
        if(h->file->owner == -1) {
            h->file->owner = client->clientFd;
            return 1;
        } else if(h->file->owner == client->clientFd) {
            return 1;
        }
        // if there's place in the waiting queue
        if((h->queueTail - h->queueHead) < MAX_CLIENTS_WAITING_FOR_LOCK) {
            h->waitingQueue[h->queueTail % MAX_CLIENTS_WAITING_FOR_LOCK] = r;
            h->queueTail++;

            int pathnameLen = h->file->pathlen;
            client->pathnamePendingLock = _malloc(sizeof(char) * (h->file->pathlen + 1));
            strncpy(client->pathnamePendingLock, h->file->pathname, pathnameLen);
            client->pathnamePendingLock[pathnameLen] = '\0';
            return 0;
        }
    }

    return -1;
}

// returns 1 if lock is successfully released, -1 if request failed
static int unlockfileWithSession(session* client, char* pathname, int index, int sessionFileIndex) {
    hashEntry* h = get_hashEntry_ref(pathname, index);

    int result = -1;

    if(h != NULL) {
        // request came from the real owner
        if(h->file->owner == client->clientFd) {
            h->file->owner = -1;
            client->canWriteOnFile[sessionFileIndex] = false;
            
            // give ownership to the next one in the waiting queue skipping nulls
            while(h->queueTail != h->queueHead) {
                request* nextOwner = h->waitingQueue[h->queueHead % MAX_CLIENTS_WAITING_FOR_LOCK];
                h->queueHead++;
                if(h->queueHead >= MAX_CLIENTS_WAITING_FOR_LOCK) {
                    h->queueHead -= MAX_CLIENTS_WAITING_FOR_LOCK;
                    h->queueTail -= MAX_CLIENTS_WAITING_FOR_LOCK;
                }

                if(nextOwner == NULL) {
                    continue;
                }

                h->file->owner = nextOwner->client->clientFd;
                free(nextOwner->client->pathnamePendingLock);
                nextOwner->client->pathnamePendingLock = NULL;
                nextOwner->client->opStatus = true;
                insert_request_queue(&clientsToNotify, nextOwner);
                break;
            }

            result = 1;
        }
    }

    return result;
}

// returns 1 if lock is successfully released, -1 if request failed
static inline int unlockfile(request* r, char* pathname, int index, int sessionFileIndex) {
    return unlockfileWithSession(r->client, pathname, index, sessionFileIndex);
}

void clean_pending_lock_request(session* client, bool needToNotify) {
    char* pathnamePendingLock = client->pathnamePendingLock;
    if(pathnamePendingLock != NULL) {
        // get bucket index
        int index = hash((unsigned char*)pathnamePendingLock) % HASHTABLE_SIZE;
        hashEntry* h = get_hashEntry_ref(pathnamePendingLock, index);
        
        // it shouldn't happen to have a pending request on a non-existing file
        if(h != NULL) {
            // find request to delete
            for(int i=h->queueHead; i<h->queueTail; i++) {
                request* r = h->waitingQueue[i % MAX_CLIENTS_WAITING_FOR_LOCK];
                // found correct request
                if(r->client->clientFd == client->clientFd) {
                    h->waitingQueue[i % MAX_CLIENTS_WAITING_FOR_LOCK] = NULL;

                    free(client->pathnamePendingLock);
                    client->pathnamePendingLock = NULL;

                    if(needToNotify) {
                        client->opStatus = false;
                        insert_request_queue(&clientsToNotify, r);
                    }
                }
            }
        }

        free(pathnamePendingLock);
        client->pathnamePendingLock = NULL;
    }
}

/* 
    exposed functions
*/

void init_filesystem() {
    init_request_queue(&clientsToNotify);
    initCachingSystem();
}

void filesystem_handle_connectionClosed(session* closedClient) {
    _pthread_rwlock_wrlock(&tableLock);

    // clear pending lock request
    clean_pending_lock_request(closedClient, false);

    // unlock locked files
    for(int i=0; i<MAX_OPENED_FILES; i++) {
        char* pathname = closedClient->openedFiles[i];
        if(pathname != NULL) {
            // get bucket index
            int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;
            unlockfileWithSession(closedClient, pathname, index, i);
        }
    }

    _pthread_rwlock_unlock(&tableLock);
}

void free_filesystem() {
    freeCachingSystem();
    printStats();

    for(int i=0; i<HASHTABLE_SIZE; i++) {
        hashEntry* e = filesystem[i];
        while(e != NULL) {
            hashEntry* nextE = e->next;
            free_hashEntry(e);
            e = nextE;
        }
    }

    free_request_queue(&clientsToNotify);
}

// returns a request with a status (1 if lock was acquired, 0 if file has been deleted)
request* filesystem_getClientToNotify() {
    return remove_request_queue(&clientsToNotify);
}

void close_filesystem_pending_locks() {
    close_request_queue(&clientsToNotify);
}

fileEntry* filesystem_get_fileEntry(char* pathname) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    fileEntry* result = NULL;

    // acquire bucket lock in read mode
    _pthread_rwlock_rdlock(&tableLock);

    fileEntry* f = get_fileEntry_ref(pathname, index);

    // copy out result
    if(f != NULL) {
        result = deep_copy_file(f);
    }

    _pthread_rwlock_unlock(&tableLock);

    return result;
}

fileEntry** filesystem_get_n_fileEntry(int* dim, int n) {
    fileEntry** cacheBuffer;
    int head;
    int count = getCacheBuffer(&cacheBuffer, &head);

    if(n == 0 || n > count) {
        n = count;
    }
    
    // reservoir sampling
    int elements[n];
    for(int i=0; i<n; i++) {
        elements[i] = i;
    }

    srand(time(NULL));

    for(int i=n; i<count; i++) {
        int j = rand() % i;
        if(j < n) {
            elements[j] = i;
        }
    }
    
    *dim = n;

    fileEntry** result = _malloc(sizeof(fileEntry*) * n);
    for(int i=0; i<n; i++) {
        result[i] = deep_copy_file(cacheBuffer[(head + elements[i]) % configs.maxFileCount]);
    }
    return result;
}

bool filesystem_fileExists(char* pathname) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    // acquire bucket lock in read mode
    _pthread_rwlock_rdlock(&tableLock);
    
    bool result = get_fileEntry_ref(pathname, index) != NULL;

    _pthread_rwlock_unlock(&tableLock);

    return result;
}

// result = -1 if failed, 0 if the lock request is in queue, 1 if lock is immediately acquired or if O_LOCK was not set and no errors
fileEntry* filesystem_openFile(int* result, request* r, char* pathname, int flags) {
    // get bucket index
    unsigned int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    bool failed = false;
    fileEntry* fileExpelled = NULL;
    *result = 1;

    session* client = r->client;

    // choose acquire mode depending on operations to be performed
    if(!(flags & O_CREATE) && !(flags & O_LOCK)) {
        _pthread_rwlock_rdlock(&tableLock);
    } else {
        _pthread_rwlock_wrlock(&tableLock);
    }

    int indexOfFile = isFileOpened(client, pathname);

    // if file already opened and O_CREATE -> error
    if(indexOfFile != -1 && (flags & O_CREATE)) {
        failed = true;
    }

    
    int descriptor = getFreeFileDescriptor(client);
    if(descriptor == -1) {
        failed = true;
    }

    if((flags & O_CREATE) && !failed) {
        // prepare dummy content
        int pathnameLen = strlen(pathname);
        int bufSize = pathnameLen + 1;
        char* buf = _malloc(sizeof(char) * bufSize);
        memcpy(buf, pathname, pathnameLen);
        buf[pathnameLen] = '\0';

        fileEntry* f = _malloc(sizeof(fileEntry));
        f->owner = -1;
        f->buf = buf;
        f->bufLen = bufSize;
        f->pathname = buf;
        f->pathlen = pathnameLen;
        f->content = buf + pathnameLen;
        f->length = 0;

        // handle caching
        fileExpelled = handleInsertion(f);

        if(fileExpelled != NULL) {
            // get bucket index
            unsigned int expelledIndex = hash((unsigned char*)fileExpelled->pathname) % HASHTABLE_SIZE;
            remove_hashEntry_ref(fileExpelled->pathname, expelledIndex, true);
        }
        

        failed = !put_fileEntry_ref(f, index);

        if(failed) {
            free(buf);
            free(f);
        }
    } else {
        failed = failed && (get_fileEntry_ref(pathname, index) == NULL);
    }

    if((flags & O_LOCK) && !failed) {
        *result = lockfile(r, pathname, index, descriptor);

        if(*result != 1) {
            failed = true;
        }
    }

    if(!failed) {
        if(get_fileEntry_ref(pathname, index) != NULL) {
            int pathLen = strlen(pathname);
            client->openedFiles[descriptor] = _malloc(sizeof(char) * (pathLen + 1));
            strncpy(client->openedFiles[descriptor], pathname, pathLen);
            client->openedFiles[descriptor][pathLen] = '\0';
            
            if((flags & O_CREATE) && (flags & O_LOCK)) {
                client->canWriteOnFile[descriptor] = true;
            }
        } else {
            failed = true;
        }
    }

    _pthread_rwlock_unlock(&tableLock);

    *result = failed ? -1 : *result;

    // printStats();

    return fileExpelled;
}

fileEntry** filesystem_writeFile(int* result, int* dim, request* r, fileEntry* f) {
    // get bucket index
    int index = hash((unsigned char*)f->pathname) % HASHTABLE_SIZE;

    session* client = r->client;

    fileEntry** filesToRemove = NULL; 
    *dim = 0;

    *result = -1;

    // acquire bucket lock in write mode
    _pthread_rwlock_wrlock(&tableLock);

    fileEntry* fileToWrite = get_fileEntry_ref(f->pathname, index);
    if(fileToWrite != NULL && fileToWrite->owner == client->clientFd) {
        int descriptor = isFileOpened(client, f->pathname);
        if(descriptor != -1 && client->canWriteOnFile[descriptor]) {

            // update cache
            filesToRemove = handleEdit(dim, fileToWrite, f->length);
            
            // remove files from filesystem before the insertion
            for(int i=0; i<*dim; i++) {
                // get bucket index
                int removeIndex = hash((unsigned char*)filesToRemove[i]->pathname) % HASHTABLE_SIZE;
                remove_hashEntry_ref(filesToRemove[i]->pathname, removeIndex, true);
            }

            // shallow copy file
            free(fileToWrite->buf);

            fileToWrite->buf = f->buf;
            fileToWrite->bufLen = f->bufLen;
            fileToWrite->content = f->content;
            fileToWrite->length = f->length;
            fileToWrite->pathname = f->pathname;

            client->canWriteOnFile[descriptor] = false;

            *result = 1;
        }
    }

    _pthread_rwlock_unlock(&tableLock);

    // printStats();

    return filesToRemove;
}

fileEntry** filesystem_appendToFile(int* result, int* dim, request* r, fileEntry* f) {
    // get bucket index
    int index = hash((unsigned char*)f->pathname) % HASHTABLE_SIZE;

    session* client = r->client;

    fileEntry** filesToRemove = NULL;
    *dim = 0;

    *result = -1;

    // acquire bucket lock in write mode
    _pthread_rwlock_wrlock(&tableLock);

    int descriptor = isFileOpened(client, f->pathname);
    if(descriptor != -1) {
        fileEntry* fileToEdit = get_fileEntry_ref(f->pathname, index);
        if(fileToEdit != NULL) {
            // calculate offsets
            int pathOffset = fileToEdit->pathname - fileToEdit->buf;
            int contentOffset = fileToEdit->content - fileToEdit->buf;

            // calculate new sizes
            int newSize = fileToEdit->bufLen + f->length;
            int newContentLength = fileToEdit->length + f->length;

            // update cache
            filesToRemove = handleEdit(dim, fileToEdit, newContentLength);
            
            // remove files from filesystem before the insertion
            for(int i=0; i<*dim; i++) {
                // get bucket index
                int removeIndex = hash((unsigned char*)filesToRemove[i]->pathname) % HASHTABLE_SIZE;
                remove_hashEntry_ref(filesToRemove[i]->pathname, removeIndex, true);
            }

            // resize underlying buffer and copy content
            fileToEdit->buf = _realloc(fileToEdit->buf, sizeof(char) * (newSize + 1));
            fileToEdit->bufLen = newSize;
            fileToEdit->buf[newSize] = '\0';
            
            // fix pointers changed due to realloc
            fileToEdit->pathname = fileToEdit->buf + pathOffset;
            fileToEdit->content = fileToEdit->buf + contentOffset;

            memcpy(fileToEdit->content + fileToEdit->length, f->content, f->length);
            fileToEdit->length = newContentLength;
            
            client->canWriteOnFile[descriptor] = false;

            *result = 1;
        }
    }

    _pthread_rwlock_unlock(&tableLock);

    // printStats();

    return filesToRemove;
}

int filesystem_lockAcquire(request* r, char* pathname) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    session* client = r->client;

    int result = -1;

    // acquire bucket lock in write mode
    _pthread_rwlock_wrlock(&tableLock);
    
    int fileIndex = client->isFileOpened(client, pathname);
    if(fileIndex != -1) {
        result = lockfile(r, pathname, index, fileIndex);
    }

    _pthread_rwlock_unlock(&tableLock);

    return result;
}

int filesystem_lockRelease(request* r, char* pathname) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    session* client = r->client;

    int result = -1;

    // acquire bucket lock in write mode
    _pthread_rwlock_wrlock(&tableLock);
    
    int fileIndex = client->isFileOpened(client, pathname);
    if(fileIndex != -1) {
        result = unlockfile(r, pathname, index, fileIndex);
        client->canWriteOnFile[fileIndex] = false;
    }

    _pthread_rwlock_unlock(&tableLock);

    return result;
}

int filesystem_removeFile(request* r, char* pathname) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    session* client = r->client;

    int result = -1;

    // acquire bucket lock in write mode
    _pthread_rwlock_wrlock(&tableLock);
    

    int descriptor = isFileOpened(client, pathname);
    if(descriptor != -1) {
        hashEntry* h = get_hashEntry_ref(pathname, index);

        if(h != NULL) {
            // request came from the real owner
            if(h->file->owner == client->clientFd) {
                client->canWriteOnFile[descriptor] = false;
                
                // notify all waiting file of the removal
                while(h->queueTail != h->queueHead) {
                    request* nextOwner = h->waitingQueue[h->queueHead % MAX_CLIENTS_WAITING_FOR_LOCK];
                    h->queueHead++;
                    if(h->queueHead >= MAX_CLIENTS_WAITING_FOR_LOCK) {
                        h->queueHead -= MAX_CLIENTS_WAITING_FOR_LOCK;
                        h->queueTail -= MAX_CLIENTS_WAITING_FOR_LOCK;
                    }

                    if(nextOwner == NULL) {
                        continue;
                    }

                    free(nextOwner->client->pathnamePendingLock);
                    nextOwner->client->pathnamePendingLock = NULL;
                    nextOwner->client->opStatus = false;

                    insert_request_queue(&clientsToNotify, nextOwner);
                }

                handleRemoval(h->file);
                remove_hashEntry_ref(pathname, index, false);

                result = 1;
            }
        }
    }

    _pthread_rwlock_unlock(&tableLock);

    // printStats();

    return result;
}