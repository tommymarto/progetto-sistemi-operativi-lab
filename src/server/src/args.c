#define _POSIX_C_SOURCE 200809L

#include <args.h>

#include <logging.h>
#include <stringutils.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#define logFoundWithParams "found option %s with argument %s"

extern int errno;
extern int logging_level;

// global app configs
config configs = {
    .cachePolicy = DEFAULT_CACHE_POLICY,
    .socketFileName =  DEFAULT_SOCKET_FILENAME,
    .nThreadWorkers = DEFAULT_N_THREAD_WORKERS,
    .maxMemorySize = DEFAULT_MAX_MEMORY_SIZE_MB,
    .maxFileCount = DEFAULT_MAX_FILE_COUNT,
    .maxFileSize = DEFAULT_MAX_FILE_SIZE,
    .maxClientsConnected = DEFAULT_MAX_CLIENTS,
    .logVerbosity = DEFAULT_LOG_VERBOSITY,
    .logFileOutput = NULL
};

int fpeek(FILE *stream) {
    int c = fgetc(stream);
    ungetc(c, stream);
    return c;
}

// parse a configuration file of the following format:
// OPTION=VALUE
// with maxlen(OPTION) = maxlen(VALUE) = 127
void parseFileArguments(char* configFileName, config* configs) {
    FILE *configFile = fopen(configFileName, "r");
    
    // if can't open the file fallback to default configuration
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
        // skip comment
        if(fpeek(configFile) == '#') {
            fscanf(configFile, "%*[^\n]\n");
            continue;
        }

        // if fscanf has found 2 elements then, edit the right config 
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

void handleConfiguration(int argc, char* argv[]) {
    // read configuration
    char* configFile = DEFAULT_CONFIGURATION_FILE;

    if(argc >= 2) {
        log_info("using command line argument as configuration file path");
        configFile = argv[1];
    } else {
        // default implementation reads config file in the same folder of the executable
        log_info("using default configuration file path");
        
        char buff[PATH_MAX];
        int len = strlen(DEFAULT_CONFIGURATION_FILE);
        ssize_t sz = readlink("/proc/self/exe", buff, sizeof(buff));
        if(sz == -1) {
            log_error("cannot read current path, using default configuration");
            return;
        } else {
            // delete executable name from path
            while(buff[--sz] != '/') {}

            // check the full path of the config file is a valid path (+2 because of the / between dir and the filename and the terminating \0)
            if(len + sz + 2 >= PATH_MAX) {
                log_error("cannot read current path, using default configuration");
                return;
            } else {
                // if everything ok, build config filename else fallback to config file in the caller folder
                strcpy(buff + sz, "/" DEFAULT_CONFIGURATION_FILE);
                buff[sz + len + 2] = '\0';
                configFile = buff;
            }
        }
    }

    // config log file
    configs.logFileOutput = fopen("serverLog.txt", "w");
    if(configs.logFileOutput == NULL) {
        log_error("unable to open logging file. Proceeding without operation logging...");
    }

    log_info("trying to read configuration file %s", configFile);
    parseFileArguments(configFile, &configs);

    // handle max fd_set size and the special value 0
    configs.maxClientsConnected = configs.maxClientsConnected == 0 ? DEFAULT_MAX_CLIENTS : MIN(configs.maxClientsConnected, DEFAULT_MAX_CLIENTS);
    
    // convert from MBytes to bytes
    configs.maxMemorySize *= 1024 * 1024;
    // handle file size misconfiguration
    configs.maxFileSize = MIN(configs.maxFileSize, configs.maxMemorySize);

    // handle verbosity level
    switch (configs.logVerbosity) {
        case 4: {
            logging_level = FATAL | ERROR | WARN | REPORT | OPERATION | INFO;
            break;
        }
        case 3: {
            logging_level = FATAL | ERROR | WARN | REPORT | OPERATION;
            break;
        }
        case 2: {
            logging_level = FATAL | ERROR | WARN | REPORT;
            break;
        }
        case 1: {
            logging_level = FATAL | ERROR | REPORT;
            break;
        }
        case 0: {
            logging_level = 0;
            break;
        }
        default: {
            log_error("invalid argument for logging verbosity. Proceeding with the default value");
        }
    }
}