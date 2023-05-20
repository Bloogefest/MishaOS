#pragma once

struct interrupt_frame;

__attribute__((interrupt))
void general_protection_fault_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void double_fault_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void master_mask_port_isr(struct interrupt_frame* frame);