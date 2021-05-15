#pragma once

typedef struct request request;

struct request {
    char* content;
    char type;
    char* extra;
    char* dir;
    int index;


    void (*free)(request* self);
    void (*set_dir)(request* self, char* newDir);
};

void free_request(request* self);
void set_request_directory(request* self, char* newDir);

request* new_request_without_dir(char type, char* content, char* extra, int index);
request* new_request(char type, char* content, char* extra, char* dir, int index);