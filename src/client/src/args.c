#define _POSIX_C_SOURCE 200112L
#include <args.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <data-structures.h>
#include <stringutils.h>
#include <logging.h>

const char* logFoundWithParams = "found option -%c with parameter %s";
const char* logFound = "found option -%c";


bool helpFlag = false;
bool logFlag = false;
string* socketFileName = NULL;


extern char* optarg;
extern int optind, opterr, optopt;

/*
    multiple arguments are space separated without any whitespace in between

    -w dirname[,n=0]
    -W file1[,file2]
    -d dirname
    -D dirname
    -r file1[,file2]
    -R [n=0]
    -t time
    -l file1[,file2]
    -u file1[,file2]
    -c file1[,file2]
*/

void handleSingleParameter(vector_request* requests, char request_type, char* arg) {
    requests->append(requests, new_request(request_type, arg));
}

void handleMultipleParameters(vector_request* requests, char request_type, char* args) {
    vector_string* v_args = char_array_split(args, strlen(args), ",");
    for(int i=0; i<v_args->size; i++) {
        handleSingleParameter(requests, request_type, v_args->get(v_args, i)->content);
    }
    v_args->free(v_args);
}

void* parseCommandLineArguments(int argc, char *argv[]) {
    vector_request* requests = new_vector_request();

    int opt;
    while ((opt = getopt(argc, argv, ":hpf:w:W:d:D:r:R:t:l:u:c:")) != -1)
    {
        switch (opt) {
            case '?': {
                printf("l'opzione '-%c' non e' gestita\n", optopt);
                break;
            }
            case ':': {
                if(optopt == 'R') {
                    handleSingleParameter(requests, opt, "0");
                } else {
                    printf("l'opzione '-%c' richiede un argomento\n", optopt);
                }
                break;
            } 
            case 'h': {
                log_info_stdout(logFound, 'h');
                helpFlag = true;
                break;
            }
            case 'p': {
                log_info_stdout(logFound, 'p');
                logFlag = true;
                break;
            }
            case 'f': {
                log_info_stdout(logFoundWithParams, opt, optarg);
                socketFileName = new_string(optarg);
                break;
            }
            case 'w': 
            case 'W':
            case 'r':
            case 'l':
            case 'u':
            case 'c': {
                log_info_stdout(logFoundWithParams, opt, optarg);
                handleMultipleParameters(requests, opt, optarg);
                break;
            }
            case 'd':
            case 'D':
            case 'R':
            case 't': {
                log_info_stdout(logFoundWithParams, opt, optarg);
                handleSingleParameter(requests, opt, optarg);
                break;
            }
            default: {
                printf("unknows");
            }
        }
    }
    
    for (int i=0; i<requests->size; i++) {
        printf("request #%d: option:%c, content:%s\n", i, requests->get(requests, i)->option, requests->get(requests, i)->content);
    }

    requests->free(requests);
}
