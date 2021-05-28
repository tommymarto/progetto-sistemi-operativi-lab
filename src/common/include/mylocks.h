#pragma once

#include <pthread.h>

void _pthread_mutex_lock(pthread_mutex_t* mutex);
void _pthread_mutex_unlock(pthread_mutex_t* mutex);
void _pthread_mutex_init(pthread_mutex_t* mutex);
void _pthread_mutex_destroy(pthread_mutex_t* mutex);

void _pthread_rwlock_unlock(pthread_rwlock_t* lock);
void _pthread_rwlock_rdlock(pthread_rwlock_t* lock);
void _pthread_rwlock_wrlock(pthread_rwlock_t* lock);

void _pthread_cond_init(pthread_cond_t* cond);
void _pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
void _pthread_cond_signal(pthread_cond_t* cond);
void _pthread_cond_broadcast(pthread_cond_t* cond);
void _pthread_cond_destroy(pthread_cond_t* cond);