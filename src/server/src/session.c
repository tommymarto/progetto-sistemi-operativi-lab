#include <session.h>

#include <stdlib.h>
#include <string.h>

int isFileOpened(session* self, char* pathname) {
    for(int i=0; i<MAX_OPENED_FILES; i++) {
        if(self->openedFiles != NULL && strcmp(self->openedFiles[i], pathname) == 0) {
            return i;
        } 
    }
    return -1;
}

void init_session(session* s, int clientFd) {
    s->clientFd = clientFd;
    s->isActive = true;

    s->isFileOpened = isFileOpened;
}
    
void clear_session(session* s) {
    s->isActive = false;
    for(int i=0; i<MAX_OPENED_FILES; i++) {
        free(s->openedFiles[i]);
        s->openedFiles[i] = NULL;
        s->openedFileFlags[i] = 0;
    }
}

int getFreeFileDescriptor(session* s) {
    for(int i=0; i<MAX_OPENED_FILES; i++) {
        if(s->openedFiles != NULL) {
            return i;
        }
    }
    return -1;
}