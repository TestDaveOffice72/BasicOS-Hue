#pragma once

#include "lib.h"

struct interrupts {
    uint16_t idt_limit;
    struct idt_descriptor *idt_address;
    struct {
        bool has_pic;
        uint8_t local_apic_id;
        uint8_t io_apic_id;
        uint32_t *io_apic_address;
        uint32_t gsi_base;
    } apic;
};

struct kernel;
KAPI EFI_STATUS init_interrupts(struct kernel *kernel);
