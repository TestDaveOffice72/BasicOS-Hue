#pragma once
#include <efi.h>
#include <efiprot.h>
#include <stddef.h>
#include <stdbool.h>

#define NAKED __attribute__((naked))
#define DEBUG_HALT for(;;) __asm__ volatile("hlt");
#define KAPI __attribute__((ms_abi))

struct boot_state {
    EFI_HANDLE            image_handle;
    EFI_SYSTEM_TABLE      *system_table;
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
/// MUST be called while boot services are available.
KAPI EFI_STATUS init_graphics(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics);
KAPI void set_pixel(int w, int h, uint32_t rgba);
KAPI void red_screen();
KAPI void green_screen();
KAPI void blue_screen();

#define ASSERT_EFI_STATUS(x) {if(EFI_ERROR((x))) { return x; }}
