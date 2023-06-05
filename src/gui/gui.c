#include "gui.h"

#include "mouse_renderer.h"

void gui_redraw() {
//    mouse_restore_buffer();
//    mouse_fill_buffer();
    mouse_render_cursor();
}