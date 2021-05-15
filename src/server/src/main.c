#define _POSIX_C_SOURCE 200809L
#include <logging.h>
#include <args.h>
#include <configuration.h>
#include <worker.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/un.h>

#define UNIX_PATH_MAX 108
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define msgBufLen 1024
typedef struct sigaction sig_act_t;

#define DEFAULT_CONFIGURATION_FILE "config.txt"
#define DEFAULT_SOCKET_FILENAME "fileStorageSocket.sk"
#define DEFAULT_CACHE_POLICY "FIFO"
#define DEFAULT_N_THREAD_WORKERS 4
#define DEFAULT_MAX_MEMORY_SIZE_MB 128
#define DEFAULT_MAX_FILE_COUNT 1000
#define DEFAULT_MAX_CLIENTS MIN(SOMAXCONN, FD_SETSIZE)
#define DEFAULT_LOG_VERBOSITY 2

extern int logging_level;

static char* socketFileName = NULL;
static int sockfd = -1;
volatile sig_atomic_t mainExit = false;
volatile sig_atomic_t threadExit = false;

static void signalHandler(int signum) {
    mainExit = true;
    if(signum == SIGINT || signum == SIGQUIT) {
        threadExit = true;
    }
}

void updateMaxFds(int* maxFds) {
    // use fstat to find the highest opened fd
    struct stat statbuf;
    while(fstat(*maxFds, &statbuf) == -1) {
        *maxFds = *maxFds - 1;
    }
}

void cleanup() {
    log_info("final cleanup");
    log_info("flushing streams...");
    fflush(NULL);
    log_info("closing the socket...");
    if(sockfd != -1) {
        close(sockfd);
    }
    log_info("unlinking socket filename...");
    if(socketFileName != NULL) {
        unlink(socketFileName);
    }
    log_info("cleanup done");
    log_info("terminating");
}

int main(int argc, char *argv[]) {
    atexit(cleanup);
    logging_level |= INFO;
    
    // signal handling setup
    int signals[] = { SIGINT, SIGQUIT, SIGHUP };

    sig_act_t s;
    memset(&s, 0, sizeof(sig_act_t));
    s.sa_handler = signalHandler;
    
    // set action for signals
    for (int i=0; i<3; ++i) {
        if (sigaction(signals[i], &s, NULL) == -1) {
            log_error("sigaction failed for signal %d", signals[i]);
            log_error("using default signal handler");
        }
        log_info("registered signal handler for %d", signals[i]);
    }

    // read configuration
    char* configFile = DEFAULT_CONFIGURATION_FILE;
    bool skipConfig = false;
    if(argc > 2) {
        log_error("too many arguments. Use ./server [configfile]");
        exit(EXIT_FAILURE);
    } else if (argc == 2) {
        log_info("using command line argument as configuration file path");
        configFile = argv[1];
    } else {
        // default implementation reads config file in the same folder of the executable
        log_info("using default configuration file path");
        
        char buff[1024];
        int len = strlen(DEFAULT_CONFIGURATION_FILE);
        ssize_t sz = readlink("/proc/self/exe", buff, sizeof(buff));
        if(sz == -1) {
            log_error("cannot read current path");
            skipConfig = true;
        } else {
            while(buff[--sz] != '/') {}

            if(len + sz >= 1024) {
                log_error("cannot read current path");
                skipConfig = true;
            } else {
                strcpy(buff + sz, "/" DEFAULT_CONFIGURATION_FILE);
                buff[sz + len + 2] = '\0';
                configFile = buff;
            }
        }
    }

    config configs = {
        .cachePolicy = DEFAULT_CACHE_POLICY,
        .socketFileName =  DEFAULT_SOCKET_FILENAME,
        .nThreadWorkers = DEFAULT_N_THREAD_WORKERS,
        .maxMemorySize = DEFAULT_MAX_MEMORY_SIZE_MB,
        .maxFileCount = DEFAULT_MAX_FILE_COUNT,
        .maxClientsConnected = DEFAULT_MAX_CLIENTS,
        .logVerbosity = DEFAULT_LOG_VERBOSITY
    };

    if(!skipConfig) {
        log_info("trying to read configuration file %s", configFile);
        parseFileArguments(configFile, &configs);
    } else {
        log_error("unable to read configuration file...");
        log_error("proceeding with default config...");
    }
    socketFileName = configs.socketFileName;
    // handle max fd_set size and the special value 0
    configs.maxClientsConnected = configs.maxClientsConnected == 0 ? DEFAULT_MAX_CLIENTS : MIN(configs.maxClientsConnected, DEFAULT_MAX_CLIENTS);

    // socket creation
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        log_fatal("unable to create socket...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }

    // setup address
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, UNIX_PATH_MAX, "%s", socketFileName);

    // bind socketname
    unlink(socketFileName);
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        log_fatal("unable to bind socket name...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }

    // enable listening
    if(listen(sockfd, configs.maxClientsConnected) == -1) {
        log_fatal("unable to listen from socket...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }

    // spawn workers
    pthread_t workers[configs.nThreadWorkers];
    for(int i=0; i<configs.nThreadWorkers; i++) {
        if(pthread_create(workers + i, NULL, worker_start, NULL) != 0) {
            log_fatal("unable to start the worker #%d", i);
            log_fatal("exiting the program...");
            exit(EXIT_FAILURE);
        }
    }

    int _maxFds = sockfd;
    fd_set set, rdset;
    FD_ZERO(&set);
    FD_SET(sockfd, &set);

    int failedAttempts = 0;

    // main loop
    while(!mainExit) {
        rdset = set;

        // accept incoming connections
        log_info("waiting for connections and listening for incoming messages...");
        int readyDescriptors = select(_maxFds + 1, &rdset, NULL, NULL, NULL);
        if(readyDescriptors == -1) {
            log_error("select error. Failed attempts %d", ++failedAttempts);
            continue;
        } else if (readyDescriptors == 0) {
            log_info("select timeout finished");
            continue;
        }
        failedAttempts = 0;

        // handle select success
        for(int fd = 0; fd <= _maxFds; fd++) {
            if(FD_ISSET(fd, &rdset)) {
                if(fd == sockfd) {
                    int conn = accept(sockfd, NULL, 0);
                    if (conn == -1) {
                        log_error("unable to accept connection from socket");
                        continue;
                    }
                    log_info("connection accepted. Descriptor #%d", conn);
                    FD_SET(conn, &set);
                    _maxFds = MAX(_maxFds, conn);
                } else {
                    char msgBuf[msgBufLen];
                    msgBuf[msgBufLen - 1] = '\0';
                    int bytesRead = read(fd, msgBuf, msgBufLen);
                    if (bytesRead == -1) {
                        log_error("failed read from client %d", fd);
                        continue;
                    } else if(bytesRead == 0) {
                        // client has disconnected
                        log_info("connection with client %d closed", fd);
                        FD_CLR(fd, &set);
                        updateMaxFds(&_maxFds);
                        close(fd);
                    } else {
                        msgBuf[bytesRead] = '\0';
                        log_info("received: %s", msgBuf);
                    }
                }
            }
        }
    }

    // cleanup
    log_info("cleanup begin");
    log_info("waiting for workers to finish");
    for(int i=0; i<configs.nThreadWorkers; i++) {
        pthread_join(workers[i], NULL);
    }
    log_info("thread join done");
    return 0;
}