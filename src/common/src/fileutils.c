#include <fileutils.h>

#include <mymalloc.h>
#include <logging.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static void fixFileName(char* filename, int len) {
    for(int i=0; i<len; i++) {
        if(filename[i] == '/') {
            filename[i] = '_';
        }
    }

    if(filename[0] == '.') {
        filename[0] = '_';
    }
}

int readFromFile(const char* pathname, char** buf) {
    *buf = NULL;

    long fileSize;
    FILE* f = fopen(pathname, "r");
    if(f == NULL) {
        return -1;
    }
    fseek(f, 0L, SEEK_END);
    fileSize = ftell(f);
    rewind(f);

    *buf = _malloc(sizeof(char) * (fileSize + 1));
    fread(*buf, sizeof(char), fileSize, f);
    (*buf)[fileSize] = '\0';
    fclose(f);
    return fileSize;
}

void writeResultsToFile(const char* dirname, char** content, int* sizes, int dim) {
    if(dirname != NULL) {
        int dirnameSize = strlen(dirname);

        // adjust dirname
        char* correctDirName = _malloc(sizeof(char) * (dirnameSize + 1));
        snprintf(correctDirName, dirnameSize, "%s", dirname);

        if(correctDirName[dirnameSize - 1] == '/') {
            dirnameSize--;
            correctDirName[dirnameSize] = '\0';
        }
        
        log_warn(correctDirName);

        // create path buff
        char* nameBuf = _malloc(sizeof(char) * (dirnameSize + 1));
        snprintf(nameBuf, dirnameSize, "%s", correctDirName);

        log_warn(nameBuf);

        // create dir if it does not exist
        struct stat st = {0};
        if (stat(nameBuf, &st) == -1) {
            mkdir(nameBuf, 0700);
        }

        for(int i=0; i<dim; i+=2) {
            fixFileName(content[i], sizes[i]);

            log_warn(content[i]);

            // newSize = dir + / + pathname
            int newLen = dirnameSize + 1 + sizes[i] + 1;
            nameBuf = _realloc(nameBuf, sizeof(char) * (newLen + 1));
            snprintf(nameBuf, newLen, "%s/%s", correctDirName, content[i]);

            log_warn(nameBuf);

            FILE* f = fopen(nameBuf, "w");
            if(f == NULL) {
                // if file is not opened ignore
                continue;
            }

            log_info("writing to file: %s", nameBuf);
            fwrite(content[i+1], sizeof(char), sizes[i+1], f);
            fclose(f);
        }

        free(correctDirName);
        free(nameBuf);
    }
}