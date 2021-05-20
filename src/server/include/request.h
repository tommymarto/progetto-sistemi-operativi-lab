#pragma once

#include <session.h>

typedef struct request request;

struct request {
    char kind;
    int contentLen;
    char* content;
    unsigned int flags;
    session* client;

    void (*free)(request* self);
    char* (*getContent)(request* r);
};

request* new_request(char* content, int contentLen, session* client);
void free_request(request* self);

void serializeInt(char* dest, int content);
int deserializeInt(char* src);