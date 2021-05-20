#include <request.h>

#include <mymalloc.h>

void serializeInt(char* dest, int content) {
    dest[3] = (content >> 24) & 0xff;
    dest[2] = (content >> 16) & 0xff;
    dest[1] = (content >> 8) & 0xff;
    dest[0] = content & 0xff;
}

int deserializeInt(char* src) {
    return (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
}


void free_request(request* self) {
    free(self->content);
    free(self);
}

char* get_content_request(request* r) {
    return r->content + 1;
}

request* new_request(char* content, int contentLen, session* client) {
    request* r = _malloc(sizeof(request));
    r->kind = content[0];
    r->contentLen = contentLen;
    r->content = content;
    r->flags = deserializeInt(content + contentLen - 4);
    r->content[contentLen - 4] = '\0';
    r->client = client;
    
    r->free = free_request;
    r->getContent = get_content_request;

    return r;
}