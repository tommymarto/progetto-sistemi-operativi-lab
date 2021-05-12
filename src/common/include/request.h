#pragma once

typedef struct request request;

struct request {
    char* content;
    char option;

    void (*free)(request* self);
};

void free_request(request* self);
request* new_request(char option, char* content);