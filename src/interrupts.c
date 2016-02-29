#include "kernel.h"

struct IDT {
    uint16_t limit;
    uint64_t offset;
} __attribute__((packed));

struct IDTDescriptor {
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

void unknown_handler();
void int8_handler();

EFI_STATUS init_interrupts() {
    struct IDT idt = {0};
    __asm__("sidt %0":"=m"(idt));
    struct IDTDescriptor* idt_off = (struct IDTDescriptor *)idt.offset;

    struct IDTDescriptor unknown_descriptor = {
        .offset_1 = (uint64_t)unknown_handler & 0xFFFF,
        .offset_2 = (((uint64_t)unknown_handler) >> 16) & 0xFFFF,
        .offset_3 = (((uint64_t)unknown_handler) >> 32),
        .selector = 0x28,
        .reserved = 0,
        .flags = 0x8E00
    };
    for(int i = 32; i < 256; i += 1) {
        idt_off[i] = unknown_descriptor;
    }
    // FIXME: we currently use int8 handler for all exception interrupts.
    for(int i = 0; i < 20; i += 1) {
        if(i == 9) continue;
        idt_off[i].offset_1 = (uint64_t)int8_handler & 0xFFFF;
        idt_off[i].offset_2 = (((uint64_t)int8_handler) >> 16) & 0xFFFF;
        idt_off[i].offset_3 = (((uint64_t)int8_handler) >> 32);
        idt_off[i].selector = 0x28;
        idt_off[i].flags = 0x8E00;
    }

    return EFI_SUCCESS;
}

__attribute__((naked)) void unknown_handler() {
    __asm__("call green_screen; hlt");
}

__attribute__((naked)) void int8_handler() {
    __asm__("call red_screen; hlt");
}
