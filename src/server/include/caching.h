#pragma once

#include <files.h>

void initCachingSystem();
void freeCachingSystem();

int getCacheBuffer(fileEntry*** cacheBuffer, int* head);
fileEntry* handleInsertion(fileEntry* file);
void handleUsage(fileEntry* file);
fileEntry** handleEdit(int* dim, fileEntry* file, int newSize);
void handleRemoval(fileEntry* file);