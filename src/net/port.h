#pragma once

#include <stdint.h>

#define PORT_DNS 53
#define PORT_BOOTP_SERVER 67
#define PORT_BOOTP_CLIENT 68
#define PORT_NTP 123

#define PORT_OSHELPER 4950

uint16_t net_ephemeral_port();