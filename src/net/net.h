#pragma once

#include <stdint.h>

//#define NET_DEBUG

#define TRACE_LINK (1 << 0)
#define TRACE_NET (1 << 1)
#define TRACE_TRANSPORT (1 << 2)
#define TRACE_APP (1 << 3)

struct net_intf_s;

extern void(*net_post_init)(struct net_intf_s* intf);
extern uint8_t net_trace;

void net_init();
void net_poll();