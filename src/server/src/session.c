#include <session.h>

#include <stdlib.h>
#include <string.h>

#include <logging.h>

int isFileOpened(session* self, char* pathname) {
    for(int i=0; i<MAX_OPENED_FILES; i++) {
        if(self->openedFiles[i] != NULL && strcmp(self->openedFiles[i], pathname) == 0) {
            return i;
        } 
    }
    return -1;
}

void init_session(session* s) {
    s->opStatus = false;
    s->pathnamePendingLock = NULL;

    for(int i=0; i<MAX_OPENED_FILES; i++) {
        s->openedFiles[i] = NULL;
        s->canWriteOnFile[i] = false;
    }

    s->isFileOpened = isFileOpened;
}
    
void clear_session(session* s) {
    s->opStatus = false;
    for(int i=0; i<MAX_OPENED_FILES; i++) {
        free(s->openedFiles[i]);
        s->openedFiles[i] = NULL;
        s->canWriteOnFile[i] = false;
    }
    free(s->pathnamePendingLock);
    s->pathnamePendingLock = NULL;
}

int getFreeFileDescriptor(session* s) {
    for(int i=0; i<MAX_OPENED_FILES; i++) {
        if(s->openedFiles != NULL) {
            return i;
        }
    }
    return -1;
}