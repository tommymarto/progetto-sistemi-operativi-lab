#pragma once

#include <session.h>
#include <files.h>

// forward declaration
typedef struct fileEntry fileEntry;
struct fileEntry;

typedef struct request request;
struct request {
    char kind;
    unsigned int flags;
    session* client;

    fileEntry* file;

    void (*free)(request* self);
    void (*free_keep_file_content)(request* self);
};

request* new_request(char* content, int contentLen, session* client);
void free_request(request* self);