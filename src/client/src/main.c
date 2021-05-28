#include <client-data-structures.h>
#include <logging.h>
#include <fileutils.h>
#include <args.h>
#include <api.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>
// required nanosleep declaration
int nanosleep(const struct timespec *req, struct timespec *rem);

const char* helpString = "\nclient manual:\n\
    *multiple arguments are comma-separated without any whitespace in between*  \n\
\n\
    -h:                                                                         \n\
        Output usage message and exit.                                          \n\
\n\
    -p:                                                                         \n\
        Enable logs on operations.                                              \n\
\n\
    -w dirname[,n=0]                                                            \n\
        Send n files from dirname to the server.                                \n\
        If n=0 all files in dirname are sent.                                   \n\
        Examples: -w mydir,3   -w mydir                                         \n\
\n\
    -W file1[,file2]                                                            \n\
        Send all files given as arguments to the server.                        \n\
        Examples: -W myfile1,myfile2   -w myfile                                \n\
\n\
    -d dirname                                                                  \n\
        Specify the folder in which the file read are saved.                    \n\
        If -d is not used, read files are thrown away.                          \n\
        Must be used with '-r' or '-R'.                                         \n\
        Examples: -d mydir                                                      \n\
\n\
    -D dirname                                                                  \n\
        Specify the folder in which the file expelled from the server are saved.\n\
        If -D is not used, files expelled from the server are thrown away.      \n\
        Must be used with '-w' or '-W'.                                         \n\
        Examples: -D myfile1,myfile2   -w myfile                                \n\
\n\
    -r file1[,file2]                                                            \n\
        Read all files given as arguments from the server.                      \n\
        Examples: -r myfile1,myfile2   -r myfile                                \n\
\n\
    -R [n=0]                                                                    \n\
        Read n random files from the server.                                    \n\
        If n=0 all files on the server are read.                                \n\
        Examples: -R 3   -R                                                     \n\
\n\
    -t time                                                                     \n\
        Timeout before the following request (in milliseconds).                 \n\
        Examples: -t 3                                                          \n\
\n\
    -l file1[,file2]                                                            \n\
        Acquire the locks of the files given as arguments.                      \n\
        Examples: -l myfile1,myfile2   -l myfile                                \n\
\n\
    -u file1[,file2]                                                            \n\
        Release the locks of the files given as arguments.                      \n\
        Examples: -u myfile1,myfile2   -u myfile                                \n\
\n\
    -c file1[,file2]                                                            \n\
        Delete from the server the files given as arguments.                    \n\
        Examples: -c myfile1,myfile2   -c myfile                                \n\
\n";

#define logAndSkipIfOperationFailed(kind, function)                                                 \
log_last_api_call();                                                                                \
if(result == -1) {                                                                                  \
    log_error("error while processing flag '" kind "': " function " failed. Skipping request...");  \
    continue;                                                                                       \
}

#define logAndSkipIfOperationFailedWithOpBeforeContinue(kind, function, beforeContinue)                                                 \
log_last_api_call();                                                                                \
if(result == -1) {                                                                                  \
    log_error("error while processing flag '" kind "': " function " failed. Skipping request...");  \
    beforeContinue;                                                                                 \
    continue;                                                                                       \
}

// flags from command line args
#define DEFAULT_SOCKET_FILENAME "fileStorageSocket.sk"
extern string* socketFileName;
extern bool hFlag;
extern bool pFlag;
extern int tFlag;

// for logging verbosity
extern int logging_level;

// for api op info
extern api_info lastApiCall;
#define logApiString "operation: %s, state: %s, file: %s, bytesRead: %d, bytesWritten: %d, duration: %.6f"
#define log_last_api_call() log_operation(logApiString, lastApiCall.opName, lastApiCall.opStatus, lastApiCall.file, lastApiCall.bytesRead, lastApiCall.bytesWritten, (double)lastApiCall.duration / 1000)

int handleSmallWFlag(char* dirname, char* saveDir, int* remaining) {
    int result = 0; 

    DIR* dir;
    dir = opendir(dirname);
    if (dir == NULL) {
        return -1;
    }

    struct dirent* ent;
    char path[PATH_MAX];
    while((ent = readdir(dir)) != NULL && *remaining > 0) {
        if(ent->d_type == DT_DIR && ent->d_name[0] != '.') {
            // visit recursively
            snprintf(path, sizeof(path), "%s/%s", dirname, ent->d_name);
            handleSmallWFlag(path, saveDir, remaining);
        } else if(ent->d_type == DT_REG) {
            // write file
            *remaining -= 1;

            snprintf(path, sizeof(path), "%s/%s", dirname, ent->d_name);
            
            result = betterOpenFile(path, O_CREATE | O_LOCK, saveDir);
            logAndSkipIfOperationFailed("w", "openFile");
            result = writeFile(path, saveDir);
            logAndSkipIfOperationFailed("w", "writeFile");
            result = closeFile(path);
            logAndSkipIfOperationFailed("w", "closeFile");
        }
    }
    closedir(dir);

    return result;
}

void flushAll() {
    fflush(NULL);
}

int main(int argc, char *argv[]) {

    // some setup
    atexit(flushAll);
    logging_level |= INFO;

    vector_request* requests = parseCommandLineArguments(argc, argv);

    if(hFlag) {
        // handle hFlag termination
        log_info("found help flag. Printing manual and terminating...");
        printf("%s", helpString); 
    } else {

        // real main
        if(pFlag) {
            log_info("found log flag. Enabling operation logging...");
            logging_level |= OPERATION;
        }
        if(socketFileName == NULL) {
            log_info("option f not found. Using default socket filename...");
            socketFileName = new_string(DEFAULT_SOCKET_FILENAME);
        }
        // setting sleep time between requests
        struct timespec sleepTime = { .tv_sec = tFlag / 1000, .tv_nsec = ((long)tFlag % 1000) * 1000000};

        log_info("fixing requests format and checking for -d and -D bad usage...");
        expandRequests(requests);
        
        log_info("connecting on socket: %s", socketFileName->content);

        // set timeout 5 seconds from now
        struct timespec timeout;
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += 5;

        bool connectionFailed = false;

        if(openConnection(socketFileName->content, 10, timeout) == -1) {
            log_last_api_call();
            log_fatal("unable to create socket");
            log_fatal("exiting the program...");
            connectionFailed = true;
        }
        log_last_api_call();

        if(!connectionFailed) {

            for(int i=0; i<requests->size; i++) {
                // sleep
                nanosleep(&sleepTime, NULL);

                request* r = requests->get(requests, i);
                char type = r->type;
                
                int result;

                log_info("processing request '%c'", type);

                switch (type) {
                    case 'w': {
                        int remaining = r->extra;
                        if(remaining == 0) {
                            remaining = INT_MAX;
                        }

                        int contentSize = strlen(r->content);
                        if(r->content[contentSize - 1] == '/') {
                            r->content[contentSize - 1] = '\0';
                        }

                        handleSmallWFlag(r->content, r->dir, &remaining);
                        break;
                    }
                    case 'W': {
                        result = betterOpenFile(r->content, O_CREATE | O_LOCK, r->dir);
                        logAndSkipIfOperationFailed("W", "openFile");
                        result = writeFile(r->content, r->dir);
                        log_last_api_call();
                        if(result == -1) {
                            log_error("error while processing flag 'W': writeFile failed. Skipping request...");
                        }
                        result = closeFile(r->content);
                        logAndSkipIfOperationFailed("W", "closeFile");
                        break;
                    }
                    case 'r': {
                        size_t bufSize;
                        char* buf;


                        result = betterOpenFile(r->content, 0, r->dir);
                        logAndSkipIfOperationFailed("r", "openFile");
                        result = readFile(r->content, (void**)&buf, &bufSize);
                        log_last_api_call();
                        if(result == -1) {
                            log_error("error while processing flag 'r': readFile failed. Skipping request...");
                        }
                        if(result != -1) {

                            if(r->dir != NULL) {
                                // write result to file

                                // copy because filename fix works in place
                                int contentLen = strlen(r->content);
                                char* contentCopy = _malloc(sizeof(char) * (contentLen + 1));
                                memcpy(contentCopy, r->content, contentLen);
                                contentCopy[contentLen] = '\0';

                                // actual preparation + write
                                char* contents[2] = { contentCopy, buf };
                                int sizes[2] = { contentLen, bufSize };
                                writeResultsToFile(r->dir, contents, sizes, 2);

                                free(contentCopy);
                            }
                        }
                        free(buf);
                        result = closeFile(r->content);
                        logAndSkipIfOperationFailed("r", "closeFile");

                        break;
                    }
                    case 'R': {
                        result = readNFiles(r->extra, r->dir);
                        logAndSkipIfOperationFailed("R", "readNFiles");
                        break;
                    }
                    case 'l': {
                        result = betterOpenFile(r->content, 0, r->dir);
                        logAndSkipIfOperationFailed("l", "openFile");
                        result = lockFile(r->content);
                        logAndSkipIfOperationFailed("l", "lockFile");
                        break;
                    }
                    case 'u': {
                        result = unlockFile(r->content);
                        if(result == -1) {
                            log_error("error while processing flag 'u': unlockFile failed. Skipping request...");
                        }
                        result = closeFile(r->content);
                        logAndSkipIfOperationFailed("u", "closeFile");
                        break;
                    }
                    case 'c': {
                        result = betterOpenFile(r->content, O_LOCK, r->dir);
                        logAndSkipIfOperationFailed("c", "openFile");
                        result = removeFile(r->content);
                        logAndSkipIfOperationFailed("c", "removeFile");
                        break;
                    }
                    case 'a': {
                        char* buf;
                        int bufSize = readFromFile(r->content, &buf);
                        if(bufSize == -1) {
                            free(buf);
                            log_error("error while processing flag 'a': error while reading from file. Skipping request...");
                            continue;
                        }

                        result = betterOpenFile(r->content, 0, r->dir);
                        logAndSkipIfOperationFailedWithOpBeforeContinue("a", "openFile", free(buf));
                        result = appendToFile(r->content, buf, bufSize, r->dir);
                        logAndSkipIfOperationFailedWithOpBeforeContinue("a", "appendToFile", free(buf));
                        result = closeFile(r->content);
                        logAndSkipIfOperationFailedWithOpBeforeContinue("a", "closeFile", free(buf));
                        free(buf);
                        break;
                    }
                    case 'd':
                    case 'D': {
                        log_info("request '%c' already handled", type);
                        break;
                    }
                    default: {
                        log_error("unhandled request type '%c'", type);
                        break;
                    }
                }
            }

            // sleep
            nanosleep(&sleepTime, NULL);

            if(closeConnection(socketFileName->content) == -1) {
                log_fatal("error during socket close");
                log_fatal("exiting the program...");
            } else {
                log_info("connection closed");
            }
            log_last_api_call();

        }
    }

    // memory cleanup
    log_info("cleanup begin");
    if(socketFileName != NULL) {
        socketFileName->free(socketFileName);
    }
    requests->free(requests);
    log_info("cleanup done");
    log_info("terminating");
    return 0;
}