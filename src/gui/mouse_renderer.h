#pragma once

#include <stdint.h>

void mouse_fill_buffer();
void mouse_restore_buffer();
void mouse_render_cursor();
void mouse_set_cursor(uint32_t* cursor);