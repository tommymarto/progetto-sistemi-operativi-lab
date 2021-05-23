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

pthread_rwlock_t locks[HASHTABLE_SIZE] = { PTHREAD_RWLOCK_INITIALIZER };
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
            client->openedFileFlags[sessionFileIndex] |= O_LOCK;
            return 1;
        }
        // if there's place in the waiting queue
        if((h->queueTail - h->queueHead) < MAX_CLIENTS_WAITING_FOR_LOCK) {
            h->waitingQueue[h->queueTail % MAX_CLIENTS_WAITING_FOR_LOCK] = r;
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
        pthread_rwlock_destroy(&locks[i]);

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

    fileEntry* result;

    // acquire bucket lock in read mode
    pthread_rwlock_rdlock(&locks[index]);

    fileEntry* f = get_fileEntry_ref(pathname, index);

    // copy out result
    if(f != NULL) {
        result = _malloc(sizeof(fileEntry));
        memcpy(result, f, sizeof(fileEntry));

        // deep copy strings
        result->content = _malloc(sizeof(char) * (f->length + 1));
        strncpy(result->content, f->content, f->length);
        result->content[f->length] = '\0';

        result->pathname = _malloc(sizeof(char) * (f->pathlen + 1));
        strncpy(result->pathname, f->pathname, f->pathlen);
        result->pathname[f->pathlen] = '\0';
    }

    pthread_rwlock_unlock(&locks[index]);

    return result;
}

bool filesystem_fileExists(char* pathname) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    // acquire bucket lock in read mode
    pthread_rwlock_rdlock(&locks[index]);
    
    bool result = get_fileEntry_ref(pathname, index) != NULL;

    pthread_rwlock_unlock(&locks[index]);

    return result;
}

fileEntry* filesystem_openFile(int* result, request* r, char* pathname, int flags) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    bool failed = false;

    session* client = r->client;

    // acquire bucket lock in read mode
    pthread_rwlock_wrlock(&locks[index]);

    int descriptor = getFreeFileDescriptor(client);
    if(descriptor == -1) {
        failed = true;
    }

    if((flags & O_CREATE) && !failed) {
        // prepare dummy content
        int pathnameLen = strlen(pathname);
        char* buf = _malloc(sizeof(char) * (pathnameLen + 1));
        strncpy(buf, pathname, pathnameLen);
        buf[pathnameLen] = '\0';

        fileEntry* f = _malloc(sizeof(fileEntry));
        f->owner = -1;
        f->buf = buf;
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
        int lockresult = lockfile(r, pathname, index, descriptor);

        // impossible but handled anyways 
        if(lockresult != 1) {
            failed = true;
        }
    }

    if(!failed) {
        int pathLen = strlen(pathname);
        client->openedFiles[descriptor] = _malloc(sizeof(char) * (pathLen + 1));
        strncpy(client->openedFiles[descriptor], pathname, pathLen);
        
        client->openedFileFlags[descriptor] = flags;
    }

    pthread_rwlock_unlock(&locks[index]);

    *result = failed ? -1 : 0;

    return NULL;
}

int filesystem_lockAcquire(request* r, char* pathname) {
    // get bucket index
    int index = hash((unsigned char*)pathname) % HASHTABLE_SIZE;

    session* client = r->client;

    int result = -1;

    // acquire bucket lock in write mode
    pthread_rwlock_wrlock(&locks[index]);
    
    int fileIndex = client->isFileOpened(client, pathname);
    if(fileIndex != -1) {
        result = lockfile(r, pathname, index, fileIndex);
    }

    pthread_rwlock_unlock(&locks[index]);

    return result;
}