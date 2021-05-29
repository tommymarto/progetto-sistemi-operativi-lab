#include <mystring.h>

#include <mymalloc.h>
#include <string.h>
#include <stdlib.h>

void free_string(string* self) {
    free(self->content);
    free(self);
}

string* new_string(char* content) {
    string* new_string = _malloc(sizeof(string));
    int content_len = strlen(content);
    new_string->size = content_len + 1;
    new_string->content = _malloc(sizeof(char) * new_string->size);
    strncpy(new_string->content, content, content_len+1);
    new_string->content[new_string->size - 1] = '\0';

    new_string->free = free_string;
    
    return new_string;
}