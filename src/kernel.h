#pragma once
#include "lib.h"

#define DEBUG_HALT for(;;) __asm__ volatile("hlt");

struct boot_state {
    EFI_MEMORY_DESCRIPTOR *memory_map;
    uint64_t              memory_map_size;
    uint64_t              map_key;
    uint64_t              descriptor_size;
    uint32_t              descriptor_version;

};

extern struct boot_state boot_state;

/*******
 * CPU *
 *******/

struct cpu_state {
    bool has_apic;
    bool has_x2apic;
    bool has_msr;
};

extern struct cpu_state cpu_state;
KAPI EFI_STATUS init_cpu();
KAPI uint64_t cpu_read_msr(uint32_t msr /*rcx*/);
KAPI void cpu_write_msr(uint32_t msr /*rcx*/, uint64_t data/*rdx*/);

/**************
 * Interrupts *
 **************/

struct idt_descriptor {
   uint16_t offset_1;    // offset bits 0..15
   uint16_t selector;    // a code segment selector in GDT or LDT
   uint16_t flags;       // various flags, namely:
                         // 0..2: Interrupt stack table
                         // 3..7: zero
                         // 8..11: type
                         // 12: zero
                         // 13..14: descriptor privilege level
                         // 15: segment present flag
   uint16_t offset_2;    // offset bits 16..31
   uint32_t offset_3;    // offset bits 32..63
   uint32_t reserved;    // unused, set to 0
} __attribute__((packed));

struct apic_info {
    bool has_pic;
    uint8_t local_apic_id;
    uint8_t io_apic_id;
    uint32_t io_apic_address;
    uint32_t gsi_base;
};

struct interrupt_state {
    uint16_t idt_limit;
    struct idt_descriptor *idt_address;
    struct apic_info apic_info;
};

extern struct interrupt_state interrupt_state;

KAPI EFI_STATUS init_interrupts();

/************
 * Graphics *
 ************/

struct graphics_info {
    EFI_GRAPHICS_OUTPUT_PROTOCOL         *protocol;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION output_mode;
    void*                                buffer_base;
    uint64_t                             buffer_size;
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
