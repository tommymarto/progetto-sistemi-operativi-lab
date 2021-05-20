#include <files.h>

#include <mymalloc.h>
#include <string.h>
#include <math.h>

int x;

// pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
// fileEntry* filesystem;
// int size;

// unsigned long hash(unsigned char *str) {
//     unsigned long hash = 5381;
//     int c;

//     while (c = *str++) {
//         hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
//     }

//     return hash;
// }

// void free_fileEntry(fileEntry* f) {
//     free(f->content);
//     free(f->pathname);
//     pthread_mutex_destroy(&f->lock);
// }

// void free_filesystem() {
//     for(int i=0; i<size; i++) {
//         free_fileEntry(&filesystem[i]);
//     }
//     free(filesystem);
// }

// void init_filesystem(int maxFiles) {
//     size = ceil(maxFiles / 0.6);    // to keep the load factor low enough
//     filesystem = _malloc(sizeof(fileEntry*) * size);
// }

// void put_fileEntry(char* pathname, char* content) {
//     int iteration = 0, currentIndex, startIndex;
//     currentIndex = startIndex = hash(pathname) % size;

//     // look for old entry if exists
//     while(filesystem[currentIndex].isValid != NULL && strcmp(filesystem[currentIndex]->pathname, pathname) != 0) {
//         currentIndex = (startIndex + (iteration*iteration) % size) % size;
//         iteration++;
//     }

//     fileEntry* old = filesystem[currentIndex];
//     old->isValid = false;

//     // look for a place to store the new entry
//     iteration = 0;
//     currentIndex = startIndex;
//     while(filesystem[currentIndex] != NULL && !filesystem[currentIndex]->isValid) {
//         currentIndex = (startIndex + (iteration*iteration) % size) % size;
//         iteration++;
//     }


//     if(filesystem[currentIndex] != NULL) {
//         // later
//     } else {
//         fileEntry* f = _malloc(sizeof(fileEntry));
//         f->pathname = pathname;
//         f->owner = -1;
//         f->length = strlen(content);
//         f->content = content;
//         f->isValid = true;
//         pthread_mutex_init(&f->lock, NULL);
//     }
// }

// fileEntry* get_fileEntry(char* pathname) {
//     int iteration = 0, currentIndex, startIndex;
//     currentIndex = startIndex = hash(pathname) % size;

//     while(filesystem[currentIndex] != NULL && strcmp(filesystem[currentIndex]->pathname, pathname) != 0) {
//         currentIndex = (startIndex + (iteration*iteration) % size) % size;
//         iteration++;
//     }

//     fileEntry* found = filesystem[currentIndex];
//     if(found == NULL) {
//         return NULL;
//     }

//     return found->isValid ? found : NULL;
// }