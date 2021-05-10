#pragma once

#include <string.h>
#include <stdlib.h>

typedef struct request request;

struct request {
    char* content;
    char option;

    void (*free)(request* self);
};

static void free_request(request* self) {
    free(self->content);
    free(self);
}

static request* new_request(char option, char* content) {
    request* r = (request*)malloc(sizeof(request));
    r->option = option;
    
    int content_len = strlen(content);
    r->content = (char*)malloc((content_len+1)*sizeof(char));
    strncpy(r->content, content, content_len);
    r->content[content_len] = '\0';
    
    r->free = free_request;

    return r;
}