#pragma once

struct interrupt_frame;

__attribute__((interrupt))
void interrupt_handler(struct interrupt_frame* frame);