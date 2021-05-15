#include <worker.h>

#include <logging.h>
#include <stdlib.h>

void* worker_start(void* arg) {
    log_info("spawned worker");
    return NULL;
}