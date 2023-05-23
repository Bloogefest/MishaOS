#pragma once

struct interrupt_frame;

__attribute__((interrupt))
void general_protection_fault_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void double_fault_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void keyboard_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void ps2_mouse_isr(struct interrupt_frame* frame);

__attribute__((interrupt))
void pit_isr(struct interrupt_frame* frame);