#define _POSIX_C_SOURCE 200809L
#include <logging.h>
#include <args.h>
#include <worker.h>
#include <intqueue.h>
#include <mymalloc.h>
#include <files.h>
#include <comunication.h>
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
int pipeFds[2];

static char* socketFileName = NULL;
int sockfd = -1;
volatile sig_atomic_t mainExitSig = false;
volatile sig_atomic_t threadExitSig = false;
volatile sig_atomic_t sighupReceived = false;
pthread_rwlock_t threadExitSigLock = PTHREAD_RWLOCK_INITIALIZER;
typedef struct sigaction sig_act_t;


bool threadExit() {
    bool result;
    pthread_rwlock_rdlock(&threadExitSigLock);
    result = threadExitSig;
    pthread_rwlock_unlock(&threadExitSigLock);
    return result;
}

static void signalHandler(int signum) {
    if(signum == SIGINT || signum == SIGQUIT) {
        mainExitSig = true;
        threadExitSig = true;
    } else if(signum == SIGHUP) {
        sighupReceived = true;
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
    if(configs.logFileOutput != NULL) {
        fclose(configs.logFileOutput);
    }
    log_info("cleanup done");
    log_info("terminating");
    log_report("server closed");
}

int main(int argc, char *argv[]) {
    atexit(cleanup);
    logging_level |= REPORT;
    
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

    // setup pipe for worket-to-main comunication
    if(pipe(pipeFds) < 0) {
        log_fatal("unable to create pipe...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }

    // setup socket
    setupServerSocket();
    
    // setup main vars
    int _maxFds = MAX(sockfd, MAX(pipeFds[0], pipeFds[1]));
    fd_set set, rdset;
    FD_ZERO(&set);
    FD_SET(sockfd, &set);
    FD_SET(pipeFds[0], &set);
    FD_SET(pipeFds[1], &set);

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

    int clientsConnected = 0;
    int maxClientsConnected = 0;

    log_report("server up and running");

    // main loop
    while(!mainExitSig) {
        rdset = set;

        // handle sighup soft termination
        if(clientsConnected == 0 && sighupReceived) {
            mainExitSig = true;
            continue;
        }

        // accept incoming connections
        int readyDescriptors = select(_maxFds + 1, &rdset, NULL, NULL, NULL);
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

                    boring_file_log(configs.logFileOutput, "fd: %d newConnection", conn);


                    clientsConnected++;
                    maxClientsConnected = MAX(maxClientsConnected, clientsConnected);

                    log_info("connection accepted. Descriptor #%d", conn);
                    init_session(sessions + conn);
                    log_info("session initialized");

                    FD_SET(conn, &set);
                    _maxFds = MAX(_maxFds, conn);
                } else if(fd == pipeFds[0]) {
                    char msg[8];
                    readn(fd, msg, sizeof(int) * 2);

                    int msgKind = deserializeInt(msg);
                    int msgFd = deserializeInt(msg + 4);

                    switch(msgKind) {
                        case 0: { // signal client closed
                            clientsConnected--;
                            
                            boring_file_log(configs.logFileOutput, "fd: %d connectionClosed", msgFd);

                            log_info("cleaning filesystem");
                            filesystem_handle_connectionClosed(sessions + msgFd);
                            log_info("clearing session");
                            clear_session(sessions + msgFd);
                            
                            log_info("closing connection with client %d", msgFd);
                            close(msgFd);
                            break;
                        }
                        case 1: { // signal restore client fd in fdset
                            log_info("reactivating client %d", msgFd);
                            FD_SET(msgFd, &set);
                            _maxFds = MAX(_maxFds, msgFd);
                            break;
                        }
                    }
                } else {
                    // remove fd from set so nobody can read except the thread that will handle the request                    
                    FD_CLR(fd, &set);

                    // use fstat to find the highest opened fd
                    struct stat statbuf;
                    while(fstat(_maxFds, &statbuf) == -1) {
                        _maxFds--;
                    }

                    insert_int_queue(&requestQueue, fd);
                }
            }
        }
    }

    printf("\n");

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
    boring_file_log(configs.logFileOutput, "maxClientsConnected: %d", maxClientsConnected);
    log_info("terminating...");
    return 0;
}