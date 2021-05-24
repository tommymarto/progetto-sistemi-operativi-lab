#include <stringutils.h>

#include <errno.h>

vector_string* char_array_split(char* char_arr, int size, const char* delim) {
    char* str = _malloc(sizeof(char) * (size+1));
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

vector_string* string_split(string* self, const char* delim) {
    return char_array_split(self->content, self->size, delim);
}

// converts str to unsigned integers. returns true if successful, false otherwise
bool strToInt(const char* src, long* result) {
    errno = 0;
    char* end = NULL;

    // check for string validity
    if (src == NULL || strlen(src) == 0) {
        return false;
    }

    // try conversion
    *result = strtol(src, &end, 10);
    if (errno == ERANGE || (end == NULL || *end != '\0')) {
        return false;
    }

    // success
    return true;
}