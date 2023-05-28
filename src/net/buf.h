#pragma once

#include <stdint.h>

#define NET_BUF_SIZE 2048
#define NET_BUF_START 256

typedef struct net_buf_s {
    struct net_buf_s* link;
    uint8_t* start;
    uint8_t* end;
    uint32_t ref_count;
    uint32_t seq;
    uint32_t flags;
} net_buf_t;

net_buf_t* net_alloc_buf();
void net_free_buf(net_buf_t* buf);