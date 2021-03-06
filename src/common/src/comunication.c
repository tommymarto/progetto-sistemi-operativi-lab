#include <comunication.h>

#include <unistd.h>
#include <errno.h>
#include <logging.h>
#include <string.h>
#include <stdbool.h>

void serializeInt(char* dest, int content) {
    dest[3] = (content >> 24) & 0xff;
    dest[2] = (content >> 16) & 0xff;
    dest[1] = (content >> 8) & 0xff;
    dest[0] = content & 0xff;
}

int deserializeInt(char* src) {
    unsigned char* actualSrc = (unsigned char*)src;
    return (actualSrc[3] << 24) | (actualSrc[2] << 16) | (actualSrc[1] << 8) | actualSrc[0];
}

// reads n bytes from fd
ssize_t readn(int fd, char *ptr, size_t bytesToRead) {
    ssize_t bytesRead = 0;
 
    while (bytesRead < bytesToRead) {
        ssize_t currentRead = read(fd, ptr, bytesToRead - bytesRead);
        
        // handle EOF reached and error occourred
        if(currentRead == -1) {
            if(errno == EINTR) {
                continue;
            }
            log_error("%s", strerror(errno));
            return -1;
        } else if(currentRead == 0) {
            return 0;
        }

        bytesRead += currentRead;
        ptr += currentRead;
    }

    // success
    return bytesRead;
}
 
// writes n bytes from fd
ssize_t writen(int fd, char *ptr, size_t bytesToWrite) {
    ssize_t bytesWritten = 0;
 
    while (bytesWritten < bytesToWrite) {
        ssize_t currentWritten = write(fd, ptr, bytesToWrite - bytesWritten);
        
        // handle EOF reached and error occourred
        if(currentWritten == -1) {
            if(errno == EINTR) {
                continue;
            }
            return -1;
        } else if(currentWritten == 0) {
            return 0;
        }

        bytesWritten += currentWritten;
        ptr += currentWritten;
    }

    // success
    return bytesWritten;
}

ssize_t pingOk(int fd) {
    char header[4];
    serializeInt(header, 0);
    
    // forward call
    return writen(fd, header, sizeof(int));
}

ssize_t pingKo(int fd) {
    char header[4];
    serializeInt(header, -2);
    
    // forward call
    return writen(fd, header, sizeof(int));
}
