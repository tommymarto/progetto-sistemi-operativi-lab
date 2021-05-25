#pragma once

#include <files.h>
#include <request.h>

// request kind: 'o'
fileEntry** openFile(request* r, int* result, int* dim, char* pathname, int flags);
// request kind: 'r'
fileEntry** readFile(request* r, int* result, int* dim, char* pathname);
// request kind: 'n'
fileEntry** readNFiles(request* r, int* result, int* dim, int N);
// request kind: 'w'
fileEntry** writeFile(request* r, int* result, int* dim, fileEntry* file);
// request kind: 'a'
fileEntry** appendToFile(request* r, int* result, int* dim, fileEntry* file);
// request kind: 'l'
int lockFile(request* r, char* pathname);
// request kind: 'u'
int unlockFile(request* r, char* pathname);
// request kind: 'c'
int closeFile(request* r, char* pathname);
// request kind: 'R'
int removeFile(request* r, char* pathname);