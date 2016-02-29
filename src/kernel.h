#pragma once
#include <efi.h>
#include <efilib.h>
#include <efiprot.h>
#include <stddef.h>

struct boot_state {
    UINTN                 memory_map_size;
    EFI_MEMORY_DESCRIPTOR *memory_map;
    UINTN                 map_key;
    UINTN                 descriptor_size;
    UINT32                descriptor_version;
};

extern struct boot_state boot_state;


/**************
 * Interrupts *
 **************/

EFI_STATUS init_interrupts();

/************
 * Graphics *
 ************/

struct graphics_info {
    EFI_GRAPHICS_OUTPUT_PROTOCOL         *protocol;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION output_mode;
    void*                                buffer_base;
    size_t                               buffer_size;
};

extern struct graphics_info graphics_info;

#define GRAPHICS_MOST_APPROPRIATE_H 1080
#define GRAPHICS_MOST_APPROPRIATE_W 1920
EFI_STATUS init_graphics(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics);
void set_pixel(int w, int h, uint32_t rgba);

#define ASSERT_EFI_STATUS(x, n) {if(EFI_ERROR((x))) { Print(n": %r\n", x); return x; }}
