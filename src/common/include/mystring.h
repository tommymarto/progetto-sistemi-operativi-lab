#pragma once

#include <string.h>

typedef struct string string;

struct string {
    char* content;
    int size;
    
    void (*free)(string* self);
};

void free_string(string* self);
string* new_string(char* content);