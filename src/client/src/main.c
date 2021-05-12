#include <stdio.h>
#include <stdlib.h>
#include <args.h>
#include <data-structures.h>
#include <logging.h>
#include <api.h>

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

extern bool hFlag;
extern bool pFlag;
extern string* socketFileName;

extern int logging_level;

int main(int argc, char *argv[]) {
    logging_level |= INFO;
    vector_request* requests = parseCommandLineArguments(argc, argv);

    if(hFlag) {
        // handle hFlag termination
        log_info(stdout, "found help flag. Printing manual and terminating...");
        printf("%s", helpString); 
    } else {

        // real main
        if(pFlag) {
            log_info(stdout, "found log flag. Enabling operation logging...");
            logging_level |= OPERATION;
        }

        if(socketFileName != NULL) {
            printf("socketpath is %s\n", socketFileName->content);
        } else {
            printf("socketpath is null\n");
        }


        openFile(NULL, 3);
    }

    // memory cleanup
    log_info(stdout, "cleanup begin");
    if(socketFileName != NULL) {
        socketFileName->free(socketFileName);
    }
    requests->free(requests);
    log_info(stdout, "cleanup done");
    log_info(stdout, "terminating");
    return 0;
}