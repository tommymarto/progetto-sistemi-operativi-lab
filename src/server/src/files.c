#include <files.h>

#include <mymalloc.h>
#include <requestqueue.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#define HASHTABLE_SIZE 4096
#define MAX_CLIENTS_WAITING_FOR_LOCK 32

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
        }
        // if there's place in the waiting queue
        if((h->queueTail - h->queueHead) < MAX_CLIENTS_WAITING_FOR_LOCK) {
            h->waitingQueue[h->queueTail % MAX_CLIENTS_WAITING_FOR_LOCK] = r;
            h->queueTail++;
            return 0;
        }
    }

    return -1;
}

/* 
    exposed functions
*/

void init_filesystem() {
    init_request_queue(&clientsToNotify);
}

void free_filesystem() {
    for(int i=0; i<HASHTABLE_SIZE; i++) {
        hashEntry* e = filesystem[i];
        while(e != NULL) {
            hashEntry* nextE = e->next;
            free_hashEntry(e);
            e = nextE;
        }
    }
    free_queue(&clientsToNotify);
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
    pthread_rwlock_rdlock(&tableLock);

    fileEntry* f = get_fileEntry_ref(pathname, index);

    // copy out result
    if(f != NULL) {
        result = deep_copy_file(f);
    }

    pthread_rwlock_unlock(&tableLock);

    return result;
}

bool filesystem_fileExists(char* pathname) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    // acquire bucket lock in read mode
    pthread_rwlock_rdlock(&tableLock);
    
    bool result = get_fileEntry_ref(pathname, index) != NULL;

    pthread_rwlock_unlock(&tableLock);

    return result;
}

// result = -1 if failed, 0 if the lock request is in queue, 1 if lock is immediately acquired
fileEntry* filesystem_openFile(int* result, request* r, char* pathname, int flags) {
    // get bucket index
    unsigned int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    bool failed = false;
    *result = 1;

    session* client = r->client;

    // acquire bucket lock in read mode
    pthread_rwlock_wrlock(&tableLock);

    if(isFileOpened(client, pathname) == -1) {
        int descriptor = getFreeFileDescriptor(client);
        if(descriptor == -1) {
            failed = true;
        }

        if((flags & O_CREATE) && !failed) {
            // prepare dummy content
            int pathnameLen = strlen(pathname);
            int bufSize = pathnameLen + 1;
            char* buf = _malloc(sizeof(char) * bufSize);
            strncpy(buf, pathname, pathnameLen);
            buf[pathnameLen] = '\0';

            fileEntry* f = _malloc(sizeof(fileEntry));
            f->owner = -1;
            f->buf = buf;
            f->bufLen = bufSize;
            f->pathname = buf;
            f->pathlen = pathnameLen;
            f->content = buf + pathnameLen;
            f->length = 0;

            failed = !put_fileEntry_ref(f, index);

            if(failed) {
                free(buf);
                free(f);
            }
        } else {
            failed = get_fileEntry_ref(pathname, index) == NULL;
        }

        if((flags & O_LOCK) && !failed) {
            *result = lockfile(r, pathname, index, descriptor);

            if(*result != 1) {
                failed = true;
            }
        }

        if(!failed) {
            int pathLen = strlen(pathname);
            client->openedFiles[descriptor] = _malloc(sizeof(char) * (pathLen + 1));
            strncpy(client->openedFiles[descriptor], pathname, pathLen);
            client->openedFiles[descriptor][pathLen] = '\0';
            
            if((flags & O_CREATE) && (flags & O_LOCK)) {
                client->canWriteOnFile[descriptor] = true;
            }
        }
    }

    pthread_rwlock_unlock(&tableLock);

    *result = failed ? -1 : *result;

    return NULL;
}

fileEntry** filesystem_writeFile(int* result, int* dim, request* r, fileEntry* f) {
    // get bucket index
    int index = hash((unsigned char*)f->pathname) % HASHTABLE_SIZE;

    session* client = r->client;

    *result = -1;

    // acquire bucket lock in write mode
    pthread_rwlock_wrlock(&tableLock);

    fileEntry* fileToWrite = get_fileEntry_ref(f->pathname, index);
    if(fileToWrite != NULL && fileToWrite->owner == client->clientFd) {
        int descriptor = isFileOpened(client, f->pathname);
        if(descriptor != -1 && client->canWriteOnFile[descriptor]) {
            // shallow copy file
            free(fileToWrite->buf);

            fileToWrite->buf = f->buf;
            fileToWrite->bufLen = f->bufLen;
            fileToWrite->content = f->content;
            fileToWrite->length = f->length;
            fileToWrite->pathname = f->pathname;

            *result = 1;
        }
    }

    pthread_rwlock_unlock(&tableLock);

    *dim = 0;

    return NULL;
}

int filesystem_lockAcquire(request* r, char* pathname) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    session* client = r->client;

    int result = -1;

    // acquire bucket lock in write mode
    pthread_rwlock_wrlock(&tableLock);
    
    int fileIndex = client->isFileOpened(client, pathname);
    if(fileIndex != -1) {
        result = lockfile(r, pathname, index, fileIndex);
    }

    pthread_rwlock_unlock(&tableLock);

    return result;
}