#include <request.h>

#include <mymalloc.h>
#include <string.h>
#include <stdlib.h>

void free_request(request* self) {
    free(self->content);
    free(self->dir);
    free(self);
}

void set_request_directory(request* self, char* newDir) {
    free(self->dir);

    // copy dir
    int str_len = strlen(newDir);
    self->dir = _malloc(sizeof(char) * (str_len+1));
    strncpy(self->dir, newDir, str_len + 1);
    self->dir[str_len] = '\0';
}

request* new_request(char type, char* content, int extra, char* dir, int index) {
    request* r = _malloc(sizeof(request));
    r->type = type;
    r->index = index;
    r->extra = extra;
    
    int str_len;
    
    // copy content
    str_len = strlen(content);
    r->content = _malloc(sizeof(char) * (str_len+1));
    memcpy(r->content, content, str_len + 1);
    r->content[str_len] = '\0';

    r->dir = NULL;
    if(dir != NULL) {
        // copy dir
        str_len = strlen(dir);
        r->dir = _malloc(sizeof(char) * (str_len+1));
        strncpy(r->dir, dir, str_len + 1);
        r->dir[str_len] = '\0';
    }

    r->free = free_request;
    r->set_dir = set_request_directory;

    return r;
}

request* new_request_without_dir(char type, char* content, int extra, int index) {
    return new_request(type, content, extra, NULL, index);
}