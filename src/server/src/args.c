#include <args.h>

#include <logging.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

extern int errno;

#define logFoundWithParams "found option %s with argument %s"

// converts str to unsigned integers. returns true if successful, false otherwise
static inline bool strToInt(const char* src, long* result) {
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

int fpeek(FILE *stream) {
    int c = fgetc(stream);
    ungetc(c, stream);
    return c;
}

void parseFileArguments(char* configFileName, config* configs) {
    FILE *configFile = fopen(configFileName, "r");
    
    if(configFile == NULL) {
        log_error("unable to read configuration file...");
        log_error("proceeding with default config...");
        return;
    }

    log_info("scanning configuration file");
    char option[128], value[128];
    long intValue;
    while (fpeek(configFile) != EOF)
    {
        if(fpeek(configFile) == '#') {
            fscanf(configFile, "%*[^\n]\n");
            continue;
        }

        if(fscanf(configFile, "%127[^=]=%127[^\n] ", option, value) == 2) {
            if(strcmp(option, "N_THREAD_WORKERS") == 0) {
                log_info(logFoundWithParams, option, value);
                if(!strToInt(value, &intValue)) {
                    log_error("invalid parameter for option %s. Expected value to be a number", option);
                    continue;
                }
                configs->nThreadWorkers = intValue;
            } else if(strcmp(option, "MAX_MEMORY_SIZE_MB") == 0) {
                log_info(logFoundWithParams, option, value);
                if(!strToInt(value, &intValue)) {
                    log_error("invalid parameter for option %s. Expected value to be a number", option);
                    continue;
                }
                configs->maxMemorySize = intValue;
            } else if(strcmp(option, "MAX_FILES") == 0) {
                log_info(logFoundWithParams, option, value);
                if(!strToInt(value, &intValue)) {
                    log_error("invalid parameter for option %s. Expected value to be a number", option);
                    continue;
                }
                configs->maxFileCount = intValue;
            } else if(strcmp(option, "MAX_FILE_SIZE") == 0) {
                log_info(logFoundWithParams, option, value);
                if(!strToInt(value, &intValue)) {
                    log_error("invalid parameter for option %s. Expected value to be a number", option);
                    continue;
                }
                configs->maxFileSize = intValue;
            } else if(strcmp(option, "SOCKET_FILEPATH") == 0) {
                log_info(logFoundWithParams, option, value);
                strncpy(configs->socketFileName, value, CONFIGURATION_STRING_LEN);
            } else if(strcmp(option, "MAX_CLIENTS") == 0) {
                log_info(logFoundWithParams, option, value);
                if(!strToInt(value, &intValue)) {
                    log_error("invalid parameter for option %s. Expected value to be a number", option);
                    continue;
                }
                configs->maxClientsConnected = intValue;
            } else if(strcmp(option, "CACHE_POLICY") == 0) {
                log_info(logFoundWithParams, option, value);
                strncpy(configs->cachePolicy, value, CONFIGURATION_STRING_LEN);
            } else if(strcmp(option, "LOG_VERBOSITY") == 0) {
                log_info(logFoundWithParams, option, value);
                if(!strToInt(value, &intValue)) {
                    log_error("invalid parameter for option %s. Expected value to be a number", option);
                    continue;
                }
                configs->logVerbosity = intValue;
            }
        }
    }
    log_info("configuration file scan ended");
    log_info("closing the configuration file");
    fclose(configFile);
    log_info("configuration file closed");
}