#define _POSIX_C_SOURCE 200809L

#include <logging.h>
#include <args.h>
#include <worker.h>
#include <intqueue.h>
#include <mymalloc.h>
#include <mylocks.h>
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

#define failIfIsNegativeOne(x, msg) \
if(x == -1) {                       \
    log_fatal(msg);                 \
    exit(EXIT_FAILURE);             \
}                                   \

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


void writeToPipe(int kind, int fd) {
    // no need for lock, atomic write
    char pipeMsg[8];
    serializeInt(pipeMsg, kind);
    serializeInt(pipeMsg + 4, fd);

    writen(pipeFds[1], pipeMsg, sizeof(int) * 2);
}

bool threadExit() {
    bool result;
    _pthread_rwlock_rdlock(&threadExitSigLock);
    result = threadExitSig;
    _pthread_rwlock_unlock(&threadExitSigLock);
    return result;
}

static void signalHandler(int signum) {
    mainExitSig = true;
    if(signum == SIGINT || signum == SIGQUIT) {
        threadExitSig = true;
    } else if(signum == SIGHUP) {
        sighupReceived = true;
    }
}

void setSignalHandlers() {
    // signal handling setup
    int signals[] = { SIGINT, SIGQUIT, SIGHUP };

    sigset_t sigset;
    sig_act_t s;

    // temporary mask signals
    failIfIsNegativeOne(sigfillset(&sigset), strerror(errno));
    failIfIsNegativeOne(pthread_sigmask(SIG_SETMASK, &sigset, NULL), strerror(errno));

    // setup signal handler function
    memset(&s, 0, sizeof(sig_act_t));
    s.sa_handler = signalHandler;
    
    // set action for signals
    for (int i=0; i<3; ++i) {
        failIfIsNegativeOne(sigaction(signals[i], &s, NULL), strerror(errno));
        log_info("registered signal handler for %d", signals[i]);
    }

    memset(&s, 0, sizeof(sig_act_t));
    s.sa_handler = SIG_IGN;

    // ignore for sigpipe
    failIfIsNegativeOne(sigaction(SIGPIPE, &s, NULL), strerror(errno));
    log_info("registered signal handler for %d", SIGPIPE);

    // unmask
    failIfIsNegativeOne(sigemptyset(&sigset), strerror(errno));
    failIfIsNegativeOne(pthread_sigmask(SIG_SETMASK, &sigset, NULL), strerror(errno));
}

void setupServerSocket() {
    // init socket filename
    int socketFileNameLen = strlen(configs.socketFileName);
    socketFileName = _malloc(sizeof(char) * (socketFileNameLen + 1));
    strncpy(socketFileName, configs.socketFileName, socketFileNameLen + 1);
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

void* thread_select(void* arg);

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
    fd_set set;
    FD_ZERO(&set);
    FD_SET(pipeFds[0], &set);
    FD_SET(pipeFds[1], &set);

    // spawn workers
    pthread_t select_thread;
    if(pthread_create(&select_thread, NULL, thread_select, &set) != 0) {
        log_fatal(strerror(errno));
        log_fatal("unable to start the worker select_thread");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }

    pthread_t notifier;
    if(pthread_create(&notifier, NULL, worker_notify, NULL) != 0) {
        log_fatal(strerror(errno));
        log_fatal("unable to start the worker notifier");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
    
    pthread_t workers[configs.nThreadWorkers];
    for(int i=0; i<configs.nThreadWorkers; i++) {
        int* threadId = _malloc(sizeof(int));
        *threadId = i;
        if(pthread_create(workers + i, NULL, worker_start, (void*)threadId) != 0) {
            log_fatal(strerror(errno));
            log_fatal("unable to start the worker #%d", i);
            log_fatal("exiting the program...");
            exit(EXIT_FAILURE);
        }
    }

    log_report("server up and running");

    // main loop
    while(!mainExitSig) {
        if(sighupReceived) {
            break;
        }

        // accept incoming connections
        int conn = accept(sockfd, NULL, 0);
        if (conn == -1) {
            if(errno != EINTR) {
                log_error("unable to accept connection from socket");
            }
            continue;
        }

        writeToPipe(2, conn);
    }

    printf("\n");

    // cleanup
    log_info("cleanup begin");

    writeToPipe(3, 0);
    
    log_info("waiting for workers to finish");
    if(pthread_join(select_thread, NULL)) {
        log_error(strerror(errno));
    }

    close_int_queue(&requestQueue);
    close_filesystem_pending_locks();   // close notifier queue

    for(int i=0; i<configs.nThreadWorkers; i++) {
        if(pthread_join(workers[i], NULL) != 0) {
            log_error(strerror(errno));
        }
    }
    if(pthread_join(notifier, NULL)) {
        log_error(strerror(errno));
    }
    log_info("all workers finished");
    
    free_filesystem();
    
    for(int i=0; i<DEFAULT_MAX_CLIENTS; i++) {
        clear_session(sessions + i);
    }
    log_info("terminating...");
    return 0;
}

void* thread_select(void* arg) {
    // mask signals so they're handled by the main thread
    sigset_t sigset;

    failIfIsNegativeOne(sigemptyset(&sigset), strerror(errno));
    failIfIsNegativeOne(sigaddset(&sigset, SIGINT), strerror(errno));
    failIfIsNegativeOne(sigaddset(&sigset, SIGQUIT), strerror(errno));
    failIfIsNegativeOne(sigaddset(&sigset, SIGHUP), strerror(errno));
    failIfIsNegativeOne(sigaddset(&sigset, SIGPIPE), strerror(errno));
    failIfIsNegativeOne(pthread_sigmask(SIG_SETMASK, &sigset, NULL), strerror(errno));

    fd_set rdset, set = *(fd_set*)arg;

    int _maxFds = MAX(sockfd, MAX(pipeFds[0], pipeFds[1]));
    
    int clientsConnected = 0;
    int maxClientsConnected = 0;

    bool selectExit = false;

    while(!threadExit()) {
        rdset = set;

        // handle sighup soft termination
        if((clientsConnected == 0 && sighupReceived && selectExit) || (selectExit && !sighupReceived)) {
            break;
        }

        // wait for ready file descriptors
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
        for(int fd = 0; fd <= _maxFds; fd++) {
            if(FD_ISSET(fd, &rdset)) {
                if(fd == pipeFds[0]) {
                    char msg[8];
                    if(readn(fd, msg, sizeof(int) * 2) < 0) {
                        log_error("error reading from pipe");
                        continue;
                    }

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
                        case 2: { // signal connection accepted
                            boring_file_log(configs.logFileOutput, "fd: %d newConnection", msgFd);

                            log_info("connection accepted. Descriptor #%d", msgFd);
                            init_session(sessions + msgFd);
                            log_info("session initialized");

                            FD_SET(msgFd, &set);
                            _maxFds = MAX(_maxFds, msgFd);
                            
                            clientsConnected++;
                            maxClientsConnected = MAX(maxClientsConnected, clientsConnected);
                            break;
                        }
                        case 3: { // termination message from main
                            selectExit = true;
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

    // set exit condition for workers
    _pthread_rwlock_wrlock(&threadExitSigLock);
    threadExitSig = true;
    _pthread_rwlock_unlock(&threadExitSigLock);
    boring_file_log(configs.logFileOutput, "maxClientsConnected: %d", maxClientsConnected);

    return NULL;
}