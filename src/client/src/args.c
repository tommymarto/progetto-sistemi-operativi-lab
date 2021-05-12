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


bool hFlag = false;
bool pFlag = false;
string* socketFileName = NULL;


extern char* optarg;
extern int optind, opterr, optopt;

/*
    multiple arguments are comma-separated without any whitespace in between

    -h
    -p
    -f filename
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

vector_request* parseCommandLineArguments(int argc, char *argv[]) {
    vector_request* requests = new_vector_request();

    int opt;
    while ((opt = getopt(argc, argv, ":hpf:w:W:d:D:r:R:t:l:u:c:")) != -1)
    {
        switch (opt) {
            case '?': {
                log_error(stdout, "unhandled option: '-%c'\n", optopt);
                break;
            }
            case ':': {
                if(optopt == 'R') {
                    handleSingleParameter(requests, opt, "0");
                } else {
                    log_error(stdout, "option '-%c' requires an argument\n", optopt);
                }
                break;
            }
            case 'h': {
                log_info(stdout, logFound, 'h');
                hFlag = true;
                break;
            }
            case 'p': {
                log_info(stdout, logFound, 'p');
                pFlag = true;
                break;
            }
            case 'f': {
                log_info(stdout, logFoundWithParams, opt, optarg);
                socketFileName = new_string(optarg);
                break;
            }
            case 'w': 
            case 'W':
            case 'r':
            case 'l':
            case 'u':
            case 'c': {
                log_info(stdout, logFoundWithParams, opt, optarg);
                handleMultipleParameters(requests, opt, optarg);
                break;
            }
            case 'd':
            case 'D':
            case 'R':
            case 't': {
                log_info(stdout, logFoundWithParams, opt, optarg);
                handleSingleParameter(requests, opt, optarg);
                break;
            }
            default: {
                printf("unknows");
            }
        }
    }

    return requests;
}
