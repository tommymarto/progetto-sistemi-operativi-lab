#include <args.h>
#include <logging.h>
#include <string.h>

#define logFoundWithParams "found option %s with argument %s"

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
    while (fpeek(configFile) != EOF)
    {
        if(fpeek(configFile) == '#') {
            fscanf(configFile, "%*[^\n]\n");
            continue;
        }

        if(fscanf(configFile, "%127[^=]=%127[^\n] ", option, value) == 2) {
            if(strcmp(option, "N_THREAD_WORKERS") == 0) {
                log_info(logFoundWithParams, option, value);
            } else if(strcmp(option, "MAX_MEMORY_SIZE_MB") == 0) {
                log_info(logFoundWithParams, option, value);
            
            } else if(strcmp(option, "MAX_FILES") == 0) {
                log_info(logFoundWithParams, option, value);
            
            } else if(strcmp(option, "SOCKET_FILEPATH") == 0) {
                log_info(logFoundWithParams, option, value);
            
            } else if(strcmp(option, "MAX_CLIENTS") == 0) {
                log_info(logFoundWithParams, option, value);
            
            } else if(strcmp(option, "CACHE_POLICY") == 0) {
                log_info(logFoundWithParams, option, value);
            
            } else if(strcmp(option, "LOG_VERBOSITY") == 0) {
                log_info(logFoundWithParams, option, value);
            }
        }
    }
    log_info("configuration file scan ended");
    log_info("closing the configuration file");
    fclose(configFile);
    log_info("configuration file closed");
}