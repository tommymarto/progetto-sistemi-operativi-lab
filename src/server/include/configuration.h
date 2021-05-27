#pragma once

#include <stdio.h>

#define CONFIGURATION_STRING_LEN 128

typedef struct config {
    char socketFileName[128];
    char cachePolicy[128];
    int nThreadWorkers;
    int maxMemorySize;
    int maxFileCount;
    int maxFileSize;
    int maxClientsConnected;
    int logVerbosity;
    FILE* logFileOutput;
} config;