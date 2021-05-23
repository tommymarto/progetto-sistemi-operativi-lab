#include <request.h>

#include <mymalloc.h>
#include <comunication.h>

void free_request(request* self) {
    free(self->file->buf);
    free(self->file);
    free(self);
}

void free_request_keep_file(request* self) {
    free(self);
}

request* new_request(char* content, int contentLen, session* client) {
    char* msg = content;

    request* r = _malloc(sizeof(request));

    // deserialize kind
    r->kind = content[0];
    r->client = client;
    msg += 1;

    // fill file
    fileEntry* f = _malloc(sizeof(fileEntry));
    f->buf = content;
    f->owner = -1;

    // deserialize path
    f->pathlen = deserializeInt(msg);
    msg += sizeof(int);
    f->pathname = msg;
    msg += f->pathlen;
    msg[0] = '\0';
    msg += 1;

    // deserialize content
    f->length = deserializeInt(msg);
    msg += sizeof(int);
    f->content = msg;
    msg += f->length;
    
    // deserialize flags
    r->flags = deserializeInt(msg);
    
    // fix content string
    content[contentLen - 4] = '\0';

    // associate file-request
    r->file = f;
    
    r->free = free_request;
    r->free_keep_file = free_request_keep_file;

    return r;
}