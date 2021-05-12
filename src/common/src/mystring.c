#include <mystring.h>
#include <string.h>
#include <stdlib.h>

void free_string(string* self) {
    free(self->content);
    free(self);
}

string* new_string(char* content) {
    string* new_string = (string*)malloc(sizeof(string));
    int content_len = strlen(content);
    new_string->size = content_len + 1;
    new_string->content = (char*)malloc(new_string->size*sizeof(char));
    strncpy(new_string->content, content, content_len);
    new_string->content[new_string->size - 1] = '\0';

    new_string->free = free_string;
    
    return new_string;
}