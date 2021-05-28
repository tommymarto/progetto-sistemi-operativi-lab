#include <mylocks.h>

#include <logging.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void _pthread_mutex_lock(pthread_mutex_t* mutex) {
    if(pthread_mutex_lock(mutex) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_mutex_lock failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}

void _pthread_mutex_unlock(pthread_mutex_t* mutex) {
    if(pthread_mutex_unlock(mutex) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_mutex_unlock failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}

void _pthread_mutex_init(pthread_mutex_t* mutex) {
    if(pthread_mutex_init(mutex, NULL) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_mutex_init failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}

void _pthread_mutex_destroy(pthread_mutex_t* mutex) {
    if(pthread_mutex_destroy(mutex) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_mutex_destroy failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}


void _pthread_rwlock_unlock(pthread_rwlock_t* lock) {
    if(pthread_rwlock_unlock(lock) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_rwlock_unlock failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }

}

void _pthread_rwlock_rdlock(pthread_rwlock_t* lock) {
    if(pthread_rwlock_rdlock(lock) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_rwlock_rdlock failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}

void _pthread_rwlock_wrlock(pthread_rwlock_t* lock) {
    if(pthread_rwlock_wrlock(lock) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_rwlock_wrlock failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}


void _pthread_cond_init(pthread_cond_t* cond) {
    if(pthread_cond_init(cond, NULL) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_cond_init failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}

void _pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    if(pthread_cond_wait(cond, mutex) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_cond_wait failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}

void _pthread_cond_signal(pthread_cond_t* cond) {
    if(pthread_cond_signal(cond) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_cond_signal failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}

void _pthread_cond_broadcast(pthread_cond_t* cond) {
    if(pthread_cond_broadcast(cond) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_cond_broadcast failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}

void _pthread_cond_destroy(pthread_cond_t* cond) {
    if(pthread_cond_destroy(cond) != 0) {
        log_fatal(strerror(errno));
        log_fatal("call to pthread_cond_destroy failed...");
        log_fatal("exiting the program...");
        exit(EXIT_FAILURE);
    }
}