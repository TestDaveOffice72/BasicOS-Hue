/*
 * The strategy for interrupts in this kernel is thus:
 *     * We will be using interrupts from 255 backwards for hardware interrupts delivered by APIC.
 *     * Interrupts 32 through 128 will be used as software interrupts, which gives us ~100
 *     interrupts; we do not use DOS-like interrupt model where function is selected through values
 *     in registers in order to:
 *          1. Reduce number of instructions necessary for common syscalls; and
 *          2. Have less clobbered registers.
 *     * The 129th interrupt is for misc software interrupts: this is where rarely used interrupts
 *     go and these use DOS-like model (i.e. number in some register) for selecting exact function.
 */
#include "kernel.h"
#include "acpi.h"
#include "uefi.h"
#include "handlers.h"

struct interrupt_state interrupt_state;

struct IDT {
    uint16_t limit;
    uint64_t offset;
} __attribute__((packed));

KAPI EFI_STATUS init_apic();

KAPI uint32_t
ioapic_read(uint8_t reg) {
    uint32_t *data = (uint32_t *)((uint64_t)interrupt_state.apic_info.io_apic_address | 0x10);
    *interrupt_state.apic_info.io_apic_address = reg;
    return *data;
}

KAPI void
ioapic_write(uint8_t reg, uint32_t val) {
    uint32_t *data = (uint32_t *)((uint64_t)interrupt_state.apic_info.io_apic_address | 0x10);
    *interrupt_state.apic_info.io_apic_address = reg;
    *data = val;
}

KAPI void
ioapic_setup(uint64_t apic_id, uint8_t from, uint8_t to)
{
    const uint32_t reg = 0x10 + from * 2;
    uint32_t high = ioapic_read(reg + 1);
    // set APIC ID
    high &= ~0xff000000;
    ioapic_write(reg + 1, high | (apic_id << 24));
    uint32_t low = ioapic_read(reg);
    low &= ~0x1ffff;
    ioapic_write(reg, low | to);
}


KAPI void set_interrupt(struct idt_descriptor* descr, void (*handler)()) {
    uint64_t address = (uint64_t)handler;
    descr->selector = 0x28;
    descr->flags = 0x8E00;
    descr->offset_1 = address & 0xffff;
    descr->offset_3 = address >> 32;
    descr->offset_2 = address >> 16;
}

KAPI EFI_STATUS
init_interrupts()
{
    EFI_STATUS status = init_apic();
    if(EFI_ERROR(status)) return status;

    struct IDT idt;
    __asm__("sidt %0":"=m"(idt));
    interrupt_state.idt_limit = idt.limit;
    interrupt_state.idt_address = (struct idt_descriptor *)idt.offset;
    uint32_t vectors = idt.limit / sizeof(struct idt_descriptor);

    // Set all interrupts to unknown handler first.
    for(int i = 0; i < vectors; i++){
        set_interrupt(interrupt_state.idt_address + i, unknown_handler);
    }

    for(int i = 32; i < 130; i++){
        set_interrupt(interrupt_state.idt_address + i, unknown_software_handler);
    }

    for(int i = 0; i < 24; i++){
        // ioapic_redirect_irq(i, 0xfe - i);
        set_interrupt(interrupt_state.idt_address + (0xfe - i), unknown_ioapic_handler);
        ioapic_setup(interrupt_state.apic_info.local_apic_id, i, 0xfe - i);
    }

    set_interrupt(interrupt_state.idt_address + 13, int13_handler);
    set_interrupt(interrupt_state.idt_address + 32, int32_handler);
    set_interrupt(interrupt_state.idt_address + 33, int33_handler);

    __asm__("int $33");
    __asm__("sti");
    return EFI_SUCCESS;
}

#define outb(p, d) {__asm__ volatile("outb %0, %1" :: "a"((uint8_t)d), "i"(p));}

static inline void
disable_pic()
{
    // Setup the old technology
    outb(0x20, 0x11);
    outb(0xa0, 0x11);
    // Setup it more (interrupt offsets)
    outb(0x21, 0xef);
    outb(0xa1, 0xf7);
    // Even more (master/slave wiring)
    outb(0x21, 4);
    outb(0xa1, 2);
    outb(0x21, 1);
    outb(0xa1, 1);
    // And tell them to shut their traps, for ever.
    outb(0xa1, 0xff);
    outb(0x21, 0xff);
}

KAPI EFI_STATUS
init_apic()
{
    const uint32_t APIC_ENABLED = 1 << 11;
    const uint32_t X2APIC_ENABLED = 1 << 10;
    const uint32_t SVR_APIC_SOFT_ENABLE = 1 << 8;
    const uint32_t FLAGS_HAS_LEGACY_PIC = 1;
    if(!cpu_state.has_apic || !cpu_state.has_x2apic) return EFI_UNSUPPORTED;

    // Collect information from ACPI (argh, annoying)
    struct XSDTHeader *apic_info = find_acpi_table((uint8_t *)"APIC");
    if(apic_info == NULL) return EFI_NOT_FOUND;
    uint32_t flags = *((uint32_t*)(apic_info + 1) + 1);
    uint32_t apic_data_len = apic_info->length - sizeof(struct XSDTHeader) - 8;
    uint8_t *apic_entry = ((uint8_t *)(apic_info + 1)) + 8;
    while(true) {
        uint8_t type = *apic_entry;
        uint8_t len = *(apic_entry + 1);
        switch(type){
            // We do not support systems which have I/O SAPIC yet.
            case 7: return EFI_UNSUPPORTED;

            case 0: { // processor local APIC
                interrupt_state.apic_info.local_apic_id = *(apic_entry + 3);
                break;
            }

            case 1: { // I/O APIC
                interrupt_state.apic_info.io_apic_id = *(apic_entry + 2);
                uint32_t io_apic_address = *(uint32_t *)(apic_entry + 4);
                interrupt_state.apic_info.io_apic_address = (uint32_t *)(uint64_t)io_apic_address;
                interrupt_state.apic_info.gsi_base = *(uint32_t *)(apic_entry + 8);
                break;
            }

            // we do not care about these.
            case 2: // namely, we probably should handle ISOs, but this is reaaly out of scope for
                    // university kernel.
            default: break;
        }
        if(len >= apic_data_len) break;
        apic_data_len -= len;
        apic_entry += len;
    }

    // Do the APIC enabling sequence
    if(flags & FLAGS_HAS_LEGACY_PIC) {
        disable_pic();
    }
    uint64_t data = cpu_read_msr(0x1B);
    cpu_write_msr(0x1B, data | APIC_ENABLED | X2APIC_ENABLED);
    // Enable soft APIC register.
    uint64_t svr_register = cpu_read_msr(0x80F);
    cpu_write_msr(0x80F, svr_register | SVR_APIC_SOFT_ENABLE);
    return EFI_SUCCESS;
}
