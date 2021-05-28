#include <mymalloc.h>

#include <logging.h>
#include <string.h>
#include <errno.h>

void* _malloc(size_t size) {
    void* ptr = malloc(size);
    if(ptr == NULL) {
        log_fatal(strerror(errno));
        log_fatal("call to malloc failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* _realloc(void* old, size_t size) {
    void* ptr = realloc(old, size);
    if(ptr == NULL) {
        log_fatal(strerror(errno));
        log_fatal("call to realloc failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
    return ptr;
}