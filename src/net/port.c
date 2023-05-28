#include "port.h"

static uint16_t next_port = 49152;

uint16_t net_ephemeral_port() {
    if (next_port == 0) {
        next_port++;
    }

    return next_port++; // TODO: Check for collision
}