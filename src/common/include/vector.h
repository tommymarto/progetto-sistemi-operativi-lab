#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


// generic type defaults to int
#ifndef GENERIC_TYPE
#define GENERIC_TYPE int
#endif

#define CONCAT(A, B) A ## B
#define GENERATE_NAME(NAME, T) CONCAT(NAME, T)

#define VECTOR_NAME GENERATE_NAME(vector_, GENERIC_TYPE)
#define RESIZE_IF_NEEDED GENERATE_NAME(resize_if_needed_, VECTOR_NAME)
#define RESIZE GENERATE_NAME(resize_, VECTOR_NAME)
#define DEFAULT_DESTRUCTOR GENERATE_NAME(_default_item_destructor_, VECTOR_NAME);

typedef struct VECTOR_NAME VECTOR_NAME;

struct VECTOR_NAME {
    GENERIC_TYPE** array;
    int capacity;
    int size;
    void            (*append)(VECTOR_NAME* self, GENERIC_TYPE* el);
    void            (*set)(VECTOR_NAME* self, GENERIC_TYPE* el, int index);
    GENERIC_TYPE*   (*get)(VECTOR_NAME* self, int index);
    void            (*free)(VECTOR_NAME* self);
    void            (*idem_destructor)(GENERIC_TYPE* el);
};

static void RESIZE(VECTOR_NAME* self, int new_capacity) {
    GENERIC_TYPE** new_array = (GENERIC_TYPE**)malloc(new_capacity*sizeof(GENERIC_TYPE*));
    memcpy(new_array, self->array, self->size*sizeof(GENERIC_TYPE*));
    for(int i=self->size; i<new_capacity; i++) new_array[i] = NULL;
    free(self->array);
    self->array = new_array;
    self->capacity = new_capacity;
}

static void RESIZE_IF_NEEDED(VECTOR_NAME* self) {
    int new_capacity = 0;
    bool needed = false;

    if (self->size >= 0.8*self->capacity) {
        needed = true;
        new_capacity = 2*self->capacity; // give it twice the capacity
    } else if (self->size <= 0.2*self->capacity && self->capacity > 10) {
        needed = true;
        new_capacity = 0.5*self->capacity; // shrink it in half the capacity
    }

    if(needed) {
        RESIZE(self, new_capacity);
    }
}

static void GENERATE_NAME(_default_item_destructor_, VECTOR_NAME)(GENERIC_TYPE* el) {
    free(el);
}

static void GENERATE_NAME(free_, VECTOR_NAME)(VECTOR_NAME* self) {
    for (int i=0; i<self->size; i++) {
        if(self->array[i] != NULL) {
            self->idem_destructor(self->array[i]);
        }
    }
    
    free(self->array);
    free(self);
}

static GENERIC_TYPE* GENERATE_NAME(get_, VECTOR_NAME)(VECTOR_NAME* self, int index) {
    if(index < 0 || index >= self->size) {
        return NULL;
    }
    return self->array[index];
}

static void GENERATE_NAME(set_, VECTOR_NAME)(VECTOR_NAME* self, GENERIC_TYPE* el, int index) {
    if(index < 0) {
        return;
    }

    if(index >= self->capacity) {
        RESIZE(self, 2*index);
    }
    self->size = MAX(index+1, self->size);
    self->array[index] = el;
    RESIZE_IF_NEEDED(self);
}

static void GENERATE_NAME(append_, VECTOR_NAME)(VECTOR_NAME* self, GENERIC_TYPE* el) {
    self->set(self, el, self->size);
}

static VECTOR_NAME* GENERATE_NAME(new_, VECTOR_NAME)() {
    VECTOR_NAME* new_vector = (VECTOR_NAME*)malloc(sizeof(VECTOR_NAME));
    new_vector->size = 0;
    new_vector->capacity = 10;
    new_vector->array = (GENERIC_TYPE**)malloc(10*sizeof(GENERIC_TYPE*));
    for(int i=0; i<new_vector->capacity; i++) new_vector->array[i] = NULL;

    new_vector->append = GENERATE_NAME(append_, VECTOR_NAME);
    new_vector->set = GENERATE_NAME(set_, VECTOR_NAME);
    new_vector->get = GENERATE_NAME(get_, VECTOR_NAME);
    
    new_vector->free = GENERATE_NAME(free_, VECTOR_NAME);
    #ifdef CUSTOM_TYPE_DESTRUCTOR
    new_vector->idem_destructor = CUSTOM_TYPE_DESTRUCTOR;
    #else
    new_vector->idem_destructor = DEFAULT_DESTRUCTOR;
    #endif
    

    return new_vector;
}

#undef GENERIC_TYPE
#undef CUSTOM_TYPE_DESTRUCTOR