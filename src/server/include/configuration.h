#pragma once

typedef struct config {
    char* socketFileName;
    char* cachePolicy;
    int nThreadWorkers;
    int maxMemorySize;
    int maxFileCount;
    int maxClientsConnected;
    int logVerbosity;
} config;