#include "buf.h"

#include "../paging.h"

extern pfa_t pfa;

static net_buf_t* net_buf_list = 0;
int net_buf_alloc_count = 0;

net_buf_t* net_alloc_buf() {
    net_buf_t* buf;

    if (!net_buf_list) {
        buf = pfa_request_page(&pfa);
    } else {
        buf = net_buf_list;
        net_buf_list = buf->link;
    }

    buf->link = 0;
    buf->start = (uint8_t*) buf + NET_BUF_START;
    buf->end = (uint8_t*) buf + NET_BUF_START;
    buf->ref_count = 1;

    ++net_buf_alloc_count;
    return buf;
}

void net_free_buf(net_buf_t* buf) {
    if (!--buf->ref_count) {
        --net_buf_alloc_count;
        buf->link = net_buf_list;
        net_buf_list = buf;
    }
}