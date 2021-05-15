#include <data-structures.h>
#include <logging.h>
#include <args.h>
#include <api.h>
#include <time.h>

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

// flags from command line args
#define DEFAULT_SOCKET_FILENAME "fileStorageSocket.sk"
string* socketFileName;
bool hFlag;
bool pFlag;

// for logging verbosity
extern int logging_level;

// for api op info
extern api_info lastApiCall;
#define logApiString "operation: %s, state: %s, file: %s, bytesRead: %d, bytesWritten: %d, duration: %.6f"
#define log_last_api_call() log_operation(logApiString, lastApiCall.opName, lastApiCall.opStatus, lastApiCall.file, lastApiCall.bytesRead, lastApiCall.bytesWritten, (double)lastApiCall.duration / 1000)

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
            logging_level |= OPERATION | MESSAGE;
        }
        if(socketFileName == NULL) {
            log_info("option f not found. Using default socket filename...");
            socketFileName = new_string(DEFAULT_SOCKET_FILENAME);
        }

        // char s[N+2]
        // FILE* ifp - fopen("inputfile", "r");
        //while((fgets(s,N+2,ifp))!=NULL)
        // fread(), fwrite()
        // fclose(ifp);

        log_info("fixing requests format and checking for -d and -D bad usage...");
        expandRequests(requests);
        
        
        log_info("connecting on socket: %s", socketFileName->content);

        // set timeout 5 seconds from now
        struct timespec timeout;
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += 5;

        if(openConnection(socketFileName->content, 10, timeout) == -1) {
            log_last_api_call();
            log_fatal("unable to create socket");
            log_fatal("exiting the program...");
            exit(EXIT_FAILURE);
        }
        log_last_api_call();

        log_info("sending stuff");
        openFile("some random stuff", 3);

        if(closeConnection(socketFileName->content) == -1) {
            log_fatal("error during socket close");
            log_fatal("exiting the program...");
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