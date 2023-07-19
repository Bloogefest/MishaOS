#include "graphics.h"

#include <video/mouse_renderer.h>

void graphics_redraw() {
    mouse_render_cursor();
}