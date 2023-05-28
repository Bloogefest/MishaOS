#pragma once

#include <stdint.h>

struct interrupt_frame;

typedef void(*irq_handler_t)(struct interrupt_frame*);

extern irq_handler_t peripheral_isrs[3];

__attribute__((interrupt))
void general_protection_fault_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void double_fault_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void page_fault_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void keyboard_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void ps2_mouse_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void pit_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void peripheral_handler0(struct interrupt_frame* frame);

__attribute__((interrupt))
void peripheral_handler1(struct interrupt_frame* frame);

__attribute__((interrupt))
void peripheral_handler2(struct interrupt_frame* frame);