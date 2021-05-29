#define _POSIX_C_SOURCE 200112L
#include <args.h>
#include <unistd.h>
#include <client-data-structures.h>
#include <stringutils.h>
#include <logging.h>

#define logFoundWithParams "found option -%c with argument %s"
#define logFound "found option -%c"
#define logAlreadyFound "option -%c already found"
#define usingDefaultValue "using default value for option -%c"
#define wrongFlagUsage "-%c flag was not used with either -%c or -%c, proceeding ignoring the flag"

string* socketFileName = NULL;
bool hFlag = false;
bool pFlag = false;
bool qFlag = false;
bool eFlag = false;
int tFlag = 0;

extern char* optarg;
extern int optind, optopt;

/*
    multiple arguments are comma-separated without any whitespace in between

    -h
    -p
    -f filename
    -q                  (quiet logs)
    -e                  (disable error logging)
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
    -a file1[,file2]    (append content of file_i to file_i)
*/

void handleSingleParameterWithExtra(vector_request* requests, char request_type, char* arg, int extra, int index) {
    requests->append(requests, new_request_without_dir(request_type, arg, extra, index));
}

void handleSingleParameter(vector_request* requests, char request_type, char* arg, int index) {
    handleSingleParameterWithExtra(requests, request_type, arg, 0, index);
}

void handleMultipleParameters(vector_request* requests, char request_type, char* args, int index) {
    vector_string* v_args = char_array_split(args, strlen(args), ",");
    for(int i=0; i<v_args->size; i++) {
        handleSingleParameter(requests, request_type, v_args->get(v_args, i)->content, index);
    }
    v_args->free(v_args);
}

bool argumentValid() {
    if(optarg[0] == '-') {
        optind -= 1;
        return false;
    }
    return true;
}

vector_request* parseCommandLineArguments(int argc, char *argv[]) {
    vector_request* requests = new_vector_request();

    bool tFound = false;

    int opt, index=0;
    while ((opt = getopt(argc, argv, ":hpqef:w:W:d:D:r:R:t:l:u:c:a:")) != -1)
    {
        switch (opt) {
            case '?': {
                log_error("unhandled option: '-%c'", optopt);
                break;
            }
            case ':': {
                if(optopt == 'R') {
                    log_info(logFound, opt);
                    log_info("using default value for option -R");
                    handleSingleParameterWithExtra(requests, opt, "", 0, index);
                } else {
                    log_error("option '-%c' requires an argument", optopt);
                }
                break;
            }
            case 'h': {
                if(hFlag) {
                    log_error(logAlreadyFound, 'h');
                    break;
                }
                log_info(logFound, 'h');
                hFlag = true;
                break;
            }
            case 'p': {
                if(pFlag) {
                    log_error(logAlreadyFound, 'p');
                    break;
                }
                log_info(logFound, 'p');
                pFlag = true;
                break;
            }
            case 'q': {
                if(qFlag) {
                    log_error(logAlreadyFound, 'q');
                    break;
                }
                log_info(logFound, 'q');
                qFlag = true;
                break;
            }
            case 'e': {
                if(eFlag) {
                    log_error(logAlreadyFound, 'e');
                    break;
                }
                log_info(logFound, 'e');
                eFlag = true;
                break;
            }
            case 't': {
                if(tFound) {
                    log_error(logAlreadyFound, 't');
                    break;
                }
                long tmp;
                if(!strToInt(optarg, &tmp)) {
                    log_error("t flag expected argument to be int");
                    break;
                }
                log_info(logFoundWithParams, 't', optarg);
                tFound = true;
                tFlag = (int)tmp;
                break;
            }
            case 'f': {
                if(socketFileName != NULL) {
                    log_error(logAlreadyFound, 'f');
                    break;
                }
                log_info(logFoundWithParams, opt, optarg);
                socketFileName = new_string(optarg);
                break;
            }
            case 'W':
            case 'r':
            case 'l':
            case 'u':
            case 'c':
            case 'a': {
                log_info(logFoundWithParams, opt, optarg);
                handleMultipleParameters(requests, opt, optarg, index);
                break;
            }
            case 'w': {
                log_info(logFoundWithParams, opt, optarg);
                vector_string* v_args = char_array_split(optarg, strlen(optarg), ",");

                // too many parameters
                if(v_args->size > 2) {
                    log_error("option -w takes at most 2 arguments. %d were given.", v_args->size);
                    break;
                }
                
                // handle extra parameter
                char* n;
                if(v_args->size == 2) {
                    n = v_args->get(v_args, 1)->content;
                } else {
                    log_info(usingDefaultValue, opt);
                    n = "0";
                }

                
                long tmp;
                if(!strToInt(n, &tmp)) {
                    log_error("option 'w' optional argument expected to be int");
                    v_args->free(v_args);
                    break;
                }

                handleSingleParameterWithExtra(requests, opt, v_args->get(v_args, 0)->content, (int)tmp, index);
                v_args->free(v_args);
                break;
            }
            case 'R': {
                long tmp;

                if(!argumentValid()) {
                    log_info(logFound, opt);
                    log_info(usingDefaultValue, opt);
                    tmp = 0;
                } else if(!strToInt(optarg, &tmp)) {
                    log_error("option 'R' optional argument expected to be int");
                    break;
                }
                
                handleSingleParameterWithExtra(requests, opt, "", (int)tmp, index);
                break;
            }
            case 'd':
            case 'D': {
                log_info(logFoundWithParams, opt, optarg);
                handleSingleParameter(requests, opt, optarg, index);
                break;
            }
            default: {
                log_error("something unhandled");
            }
        }
        index++;
    }

    return requests;
}


// handle special flags -d and -D.
// these flags are set RIGHT AFTER the request they are referred to (see examples on project specification pdf)
// I want to handle -r file1,file2 as two separate requests -r file1 and -r file2 but doing that i lose information on -d and -D flag
// the request index purpose is to identify all the separated requests that come from a single flag.
// Because I want to be able to process requests independently i need to convert a sequence like
//      -r file1 -r file2,file3 -d mydir
// in a list of requests like
//      request #1: 'r' "file1"
//      request #2: 'r' "file2" "mydir"
//      request #3: 'r' "file2" "mydir"
void expandRequests(vector_request* requests) {
    for(int i=0; i<requests->size; i++) {
        request* cur_req = requests->get(requests, i);
        bool workToDo = false;

        // check if work needs to be done
        switch (requests->get(requests, i)->type) {
            case 'd': {
                // all the requests with the same index have the same type
                char prev_type = requests->get(requests, i-1)->type;
                if(prev_type != 'r' && prev_type != 'R') {
                    log_error(wrongFlagUsage, 'd', 'r', 'R');
                    continue;
                }
                workToDo = true;
                break;
            }
            case 'D': {
                // all the requests with the same index have the same type
                char prev_type = requests->get(requests, i-1)->type;
                if(prev_type != 'w' && prev_type != 'W') {
                    log_error(wrongFlagUsage, 'D', 'w', 'W');
                    continue;
                }
                workToDo = true;
                break;
            }
        }

        // do the work
        if(workToDo) {
            // go back and set dir for all the requests with the same index
            for(int k=i-1; k>=0 && requests->get(requests, k)->index == cur_req->index-1; k--) {
                request* r = requests->get(requests, k);
                r->set_dir(r, cur_req->content);
            }
        }
    }
}