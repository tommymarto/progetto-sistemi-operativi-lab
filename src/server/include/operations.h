#pragma once

#include <files.h>
#include <request.h>

// request kind: 'o'
fileEntry** openFile(request* r, int* result, char* pathname, int flags);
// request kind: 'r'
fileEntry** readFile(request* r, int* result, char* pathname);
// request kind: 'n'
fileEntry** readNFiles(request* r, int* result, int N);
// request kind: 'w'
fileEntry** writeFile(request* r, int* result, char* pathname, char* content, int size);
// request kind: 'a'
fileEntry** appendToFile(request* r, int* result, char* pathname, char* content, int size);
// request kind: 'l'
int lockFile(request* r, char* pathname);
// request kind: 'u'
int unlockFile(request* r, char* pathname);
// request kind: 'c'
int closeFile(request* r, char* pathname);
// request kind: 'R'
int removeFile(request* r, char* pathname);