#include <session.h>

#include <string.h>

void init_session(session* s, int clientFd) {
    s->clientFd = clientFd;
    s->isActive = true;
}
    
void clear_session(session* s) {
    s->isActive = false;
}