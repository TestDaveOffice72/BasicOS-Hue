#pragma once
#include "lib.h"
#include "uefi.h"
#define GRAPHICS_MOST_APPROPRIATE_H 1080
#define GRAPHICS_MOST_APPROPRIATE_W 1920

struct graphics {
    EFI_GRAPHICS_OUTPUT_PROTOCOL         *protocol;
    uint32_t                             mode_id;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION output_mode;
    void*                                buffer_base;
    uint64_t                             buffer_size;
};

/// MUST be called while boot services are available.
KAPI EFI_STATUS init_graphics(const struct uefi *uefi, struct graphics *gs);
KAPI void set_pixel(const struct graphics *gs, int w, int h, uint32_t rgba);
KAPI void red_screen();
KAPI void green_screen();
KAPI void blue_screen();
