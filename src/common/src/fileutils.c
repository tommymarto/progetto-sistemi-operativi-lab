#define _POSIX_C_SOURCE 200809L

#include <fileutils.h>

#include <mymalloc.h>
#include <logging.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
        
        // create path buff with additional length for 'mkdir -p '
        char nameBuf[PATH_MAX + 10];
        nameBuf[0] = '\0';
        strcat(nameBuf, "mkdir -p ");
        strcat(nameBuf, dirname);

        if(nameBuf[9 + dirnameSize - 1] != '/') {
            nameBuf[9 + dirnameSize] = '/';
            nameBuf[9 + dirnameSize + 1] = '\0';
            dirnameSize++;
        }


        for(int i=0; i<dim; i+=2) {
            int contentSize = sizes[i];
            char* contentPtr = content[i];

            while(contentPtr[0] == '.' || contentPtr[0] == '/') {
                contentPtr++;
                contentSize--;
            }

            strncpy(nameBuf + 9 + dirnameSize, contentPtr, PATH_MAX - dirnameSize);

            int folderNameIndex = 9 + dirnameSize + contentSize;
            while(nameBuf[folderNameIndex] != '/') {
                folderNameIndex--;
            }

            nameBuf[folderNameIndex] = '\0';

            // call mkdir
            system(nameBuf);

            nameBuf[folderNameIndex] = '/';

            FILE* f = fopen(nameBuf + 9, "w");
            if(f == NULL) {
                // if file is not opened ignore
                continue;
            }

            log_info("writing to file: %s", nameBuf);
            fwrite(content[i+1], sizeof(char), sizes[i+1], f);
            fclose(f);
        }
    }
}