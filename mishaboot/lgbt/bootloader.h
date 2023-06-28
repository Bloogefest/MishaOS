#pragma once

#include <stdint.h>

extern volatile uint16_t dap_num_sectors;
extern volatile uint16_t dap_buf_offset;
extern volatile uint16_t dap_buf_segment;
extern volatile uint32_t dap_lba_lower;
extern volatile uint32_t dap_lba_upper;
extern volatile uint16_t drive_params_bps;
extern uint8_t disk_space[];

void enter_protected_mode();
void do_bios_call(int func, int arg);
void bios_call(char* target, uint32_t sector);