#include <stdio.h>
#include <stdlib.h>
#include <args.h>
#include <data-structures.h>
#include <logging.h>
#include <api.h>

const char* helpString = "\nclient manual:                                  \n\
    *multiple arguments are space separated without any whitespace in between*  \n\
                                                                                \n\
    -w dirname[,n=0]                                                            \n\
    -W file1[,file2]                                                            \n\
    -d dirname                                                                  \n\
    -D dirname                                                                  \n\
    -r file1[,file2]                                                            \n\
    -R [n=0]                                                                    \n\
    -t time                                                                     \n\
    -l file1[,file2]                                                            \n\
    -u file1[,file2]                                                            \n\
    -c file1[,file2]                                                            \n\
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

// typedef int (*argfuncs)(const char *const);

// int mapfunc(char c)
// {
//     switch (c)
//     {
//     case 'n':
//         return 0;
//     case 'm':
//         return 1;
//     case 'o':
//         return 2;
//     case 'h':
//         return 3;
//     }
//     return -1;
// }

// int main(int argc, char *argv[])
// {
//     argfuncs V[] = {printn, printm, printo, printh};

//     int opt;
//     while ((opt = getopt(argc, argv, "n:m:o:h")) != -1)
//     {
//         switch (opt)
//         {
//         case '?':
//             break;
//         case 'h':
//         {
//             V[mapfunc(opt)](argv[0]);
//             break;
//         }
//         default:
//         {
//             V[mapfunc(opt)](optarg);
//         }
//         }
//     }
//     return 0;
// }