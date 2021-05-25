#define _POSIX_C_SOURCE 200809L
#include <logging.h>
#include <args.h>
#include <worker.h>
#include <intqueue.h>
#include <mymalloc.h>
#include <files.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/un.h>

#define UNIX_PATH_MAX 108
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

extern int logging_level;
extern config configs;

session sessions[DEFAULT_MAX_CLIENTS];
int_queue requestQueue;

static char* socketFileName = NULL;
int sockfd = -1;
volatile sig_atomic_t mainExitSig = false;
volatile sig_atomic_t threadExitSig = false;
pthread_rwlock_t threadExitSigLock = PTHREAD_RWLOCK_INITIALIZER;
typedef struct sigaction sig_act_t;

fd_set set;
int maxFds;
pthread_mutex_t setLock = PTHREAD_MUTEX_INITIALIZER;


bool threadExit() {
    bool result;
    pthread_rwlock_rdlock(&threadExitSigLock);
    result = threadExitSig;
    pthread_rwlock_unlock(&threadExitSigLock);
    return result;
}

static void signalHandler(int signum) {
    mainExitSig = true;
    if(signum == SIGINT || signum == SIGQUIT) {
        threadExitSig = true;
    }
}

void setSignalHandlers() {
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
}

void setupServerSocket() {
    // init socket filename
    int socketFileNameLen = strlen(configs.socketFileName);
    socketFileName = _malloc(sizeof(char) * (socketFileNameLen + 1));
    strncpy(socketFileName, configs.socketFileName, socketFileNameLen);
    socketFileName[socketFileNameLen] = '\0';

    // socket creation
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
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
}

void setFdToSet(int fd) {
    pthread_mutex_lock(&setLock);
    
    FD_SET(fd, &set);
    maxFds = MAX(maxFds, fd);
    
    pthread_mutex_unlock(&setLock);
}

void clearFdFromSet(int fd) {
    pthread_mutex_lock(&setLock);
    
    FD_CLR(fd, &set);

    // use fstat to find the highest opened fd
    struct stat statbuf;
    while(fstat(maxFds, &statbuf) == -1) {
        maxFds--;
    }

    pthread_mutex_unlock(&setLock);
}

void handleSockClose(int fd) {
    pthread_mutex_lock(&setLock);

    log_info("cleaning filesystem");
    filesystem_handle_connectionClosed(sessions + fd);
    log_info("clearing session");
    clear_session(sessions + fd);
    
    log_info("closing connection with client %d", fd);
    close(fd);

    pthread_mutex_unlock(&setLock);
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
    free(socketFileName);
    log_info("cleanup done");
    log_info("terminating");
}

int main(int argc, char *argv[]) {
    atexit(cleanup);
    logging_level |= INFO;
    
    // configure signal handling
    setSignalHandlers();

    // handle server configurations
    handleConfiguration(argc, argv);

    // setup resources
    for (int i=0; i<DEFAULT_MAX_CLIENTS; i++) {
        sessions[i].clientFd = i;
    }
    init_int_queue(&requestQueue);
    init_filesystem();

    // setup socket
    setupServerSocket();
    
    // setup main vars
    maxFds = sockfd;
    fd_set rdset;
    FD_ZERO(&set);
    FD_SET(sockfd, &set);

    // spawn workers
    pthread_t notifier;
    if(pthread_create(&notifier, NULL, worker_notify, NULL) != 0) {
        log_fatal("unable to start the worker notifier");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
    
    pthread_t workers[configs.nThreadWorkers];
    for(int i=0; i<configs.nThreadWorkers; i++) {
        int* threadId = _malloc(sizeof(int));
        *threadId = i;
        if(pthread_create(workers + i, NULL, worker_start, (void*)threadId) != 0) {
            log_fatal("unable to start the worker #%d", i);
            log_fatal("exiting the program...");
            exit(EXIT_FAILURE);
        }
    }

    // main loop
    while(!mainExitSig) {
        int _maxFds;
        pthread_mutex_lock(&setLock);
        rdset = set;
        _maxFds = maxFds;
        pthread_mutex_unlock(&setLock);

        // accept incoming connections
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 10000 };
        int readyDescriptors = select(_maxFds + 1, &rdset, NULL, NULL, &timeout);
        if(readyDescriptors == -1) {
            if(errno != EINTR) {
                log_error("select error");
            }
            continue;
        } else if (readyDescriptors == 0) {
            continue;
        }

        // handle select success
        for(int fd = 0; fd <= _maxFds && !mainExitSig; fd++) {
            if(FD_ISSET(fd, &rdset)) {
                if(fd == sockfd) {
                    int conn = accept(sockfd, NULL, 0);
                    if (conn == -1) {
                        log_error("unable to accept connection from socket");
                        continue;
                    }
                    log_info("connection accepted. Descriptor #%d", conn);
                    init_session(sessions + conn);
                    log_info("session initialized");

                    setFdToSet(conn);
                } else {
                    clearFdFromSet(fd);

                    insert_int_queue(&requestQueue, fd);
                }
            }
        }
    }

    // cleanup
    log_info("cleanup begin");
    
    log_info("closing queue");
    if(!threadExit()) {
        wait_int_queue_empty(&requestQueue);
        // not wait notifier queue 'cause someone may be still waiting for a lock

        pthread_rwlock_wrlock(&threadExitSigLock);
        threadExitSig = true;
        pthread_rwlock_unlock(&threadExitSigLock);
    }
    close_int_queue(&requestQueue);
    close_filesystem_pending_locks();   // close notifier queue
    
    log_info("waiting for workers to finish");
    for(int i=0; i<configs.nThreadWorkers; i++) {
        pthread_join(workers[i], NULL);
    }
    pthread_join(notifier, NULL);
    log_info("all workers finished");
    
    free_int_queue(&requestQueue);
    free_filesystem();
    for(int i=0; i<DEFAULT_MAX_CLIENTS; i++) {
        clear_session(sessions + i);
    }
    log_info("terminating...");
    return 0;
}