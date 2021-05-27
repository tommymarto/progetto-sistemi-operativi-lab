#pragma once

#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>

void serializeInt(char* dest, int content);
int deserializeInt(char* src);

// reads n bytes from fd
ssize_t readn(int fd, char *ptr, size_t bytesToRead);

// writes n bytes from fd
ssize_t writen(int fd, char *ptr, size_t bytesToWrite);

ssize_t pingOk(int fd);
ssize_t pingKo(int fd);