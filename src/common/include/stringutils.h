#pragma once
#include "data-structures.h"

static vector_string* char_array_split(char* char_arr, int size, const char* delim) {
    char* str = (char*)malloc((size+1)*sizeof(char));
    strncpy(str, char_arr, size);
    str[size] = '\0';
    
    vector_string* result = new_vector_string();
    char* token = strtok(str, delim);
    // loop through the string to extract all other tokens
    while( token != NULL ) {
        result->append(result, new_string(token));
        token = strtok(NULL, delim);
    }
    free(str);
    return result;
}

static vector_string* string_split(string* self, const char* delim) {
    return char_array_split(self->content, self->size, delim);
}