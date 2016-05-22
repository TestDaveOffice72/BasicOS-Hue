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
#include "cpu.h"
#include "handlers.h"
#include "uefi.h"
#include "efidef.h"


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

struct IDT {
    uint16_t limit;
    uint64_t offset;
} __attribute__((packed));

KAPI EFI_STATUS init_apic(struct kernel *kernel);
KAPI void ioapic_setup(struct interrupts *is, uint64_t apic_id, uint8_t from, uint8_t to);
KAPI void set_interrupt(struct idt_descriptor* descr, void (*handler)());

KAPI EFI_STATUS
init_interrupts(struct kernel *kernel)
{
    struct interrupts *is = &kernel->interrupts;
    serial_print("\tAPIC\n");
    EFI_STATUS status = init_apic(kernel);
    if(EFI_ERROR(status)) return status;

    struct IDT idt;
    status = kernel->uefi.system_table->BootServices->AllocatePages(AllocateAnyPages,
                                                                    EfiLoaderData,
                                                                    1, &idt.offset);
    ASSERT_EFI_STATUS(status);
    idt.limit = 0xfff;
    __asm__("cli;"
            "lidt %0":"=m"(idt));
    is->idt_limit = idt.limit;
    is->idt_address = (struct idt_descriptor *)idt.offset;

    uint32_t vectors = idt.limit / sizeof(struct idt_descriptor);
    serial_print("\tSetting all to unknown handler\n");
    // Set all interrupts to unknown handler first.
    for(int i = 0; i < vectors; i++){
        set_interrupt(is->idt_address + i, unknown_handler);
    }

    serial_print("\tSetting some to unknown software handler\n");
    for(int i = 32; i < 130; i++){
        set_interrupt(is->idt_address + i, unknown_software_handler);
    }

    serial_print("\tSetting DF handler\n");
    set_interrupt(is->idt_address + 8, df_handler);
    serial_print("\tSetting GP handler\n");
    set_interrupt(is->idt_address + 13, gp_handler);
    serial_print("\tSetting PF handler\n");
    set_interrupt(is->idt_address + 14, pf_handler);
    serial_print("\tSetting UD handler\n");
    set_interrupt(is->idt_address + 6, ud_handler);

    serial_print("\tSetting IRQ1 handler\n");
    set_interrupt(is->idt_address + (0xfe - 1), irq1_handler);
    ioapic_setup(is, is->apic.local_apic_id, 1, 0xfe - 1);

    serial_print("\tSetting INT32 handler\n");
    set_interrupt(is->idt_address + 32, int32_handler);
    serial_print("\tSetting INT33 handler\n");
    set_interrupt(is->idt_address + 33, int33_handler);

    __asm__("sti");
    return EFI_SUCCESS;
}


// ------------------------------------------------------------------------------------------------
KAPI uint32_t
ioapic_read(const struct interrupts *is, uint8_t reg)
{
    uint32_t *data = (uint32_t *)((uint64_t)is->apic.io_apic_address | 0x10);
    *is->apic.io_apic_address = reg;
    return *data;
}

KAPI void
ioapic_write(const struct interrupts *is, uint8_t reg, uint32_t val)
{
    uint32_t *data = (uint32_t *)((uint64_t)is->apic.io_apic_address | 0x10);
    *is->apic.io_apic_address = reg;
    *data = val;
}

KAPI void
ioapic_setup(struct interrupts *is, uint64_t apic_id, uint8_t from, uint8_t to)
{
    const uint32_t reg = 0x10 + from * 2;
    uint32_t high = ioapic_read(is, reg + 1);
    // set APIC ID
    high &= ~0xff000000;
    ioapic_write(is, reg + 1, high | (apic_id << 24));
    uint32_t low = ioapic_read(is, reg);
    low &= ~0x1ffff;
    ioapic_write(is, reg, low | to);
}


KAPI void
set_interrupt(struct idt_descriptor* descr, void (*handler)())
{
    uint64_t address = (uint64_t)handler;
    descr->selector = 0x28;
    descr->flags = 0x8E00;
    descr->offset_1 = address & 0xffff;
    descr->offset_3 = address >> 32;
    descr->offset_2 = address >> 16;
}

static INLINE void
disable_pic()
{
    // Setup the old technology
    port_outb(0x20, 0x11);
    port_outb(0xa0, 0x11);
    // Setup it more (interrupt offsets)
    port_outb(0x21, 0xef);
    port_outb(0xa1, 0xf7);
    // Even more (master/slave wiring)
    port_outb(0x21, 4);
    port_outb(0xa1, 2);
    port_outb(0x21, 1);
    port_outb(0xa1, 1);
    // And tell them to shut their traps, for ever.
    port_outb(0xa1, 0xff);
    port_outb(0x21, 0xff);
}

KAPI EFI_STATUS
init_apic(struct kernel *kernel)
{
    const uint32_t APIC_ENABLED = 1 << 11;
    const uint32_t X2APIC_ENABLED = 1 << 10;
    const uint32_t SVR_APIC_SOFT_ENABLE = 1 << 8;
    const uint32_t FLAGS_HAS_LEGACY_PIC = 1;
    if(!kernel->cpu.has_apic || !kernel->cpu.has_x2apic) return EFI_UNSUPPORTED;

    // Collect information from ACPI (argh, annoying)
    struct XSDTHeader *apic_info = find_acpi_table(&kernel->uefi, (uint8_t *)"APIC");
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
                kernel->interrupts.apic.local_apic_id = *(apic_entry + 3);
                break;
            }

            case 1: { // I/O APIC
                kernel->interrupts.apic.io_apic_id = *(apic_entry + 2);
                uint32_t io_apic_address = *(uint32_t *)(apic_entry + 4);
                kernel->interrupts.apic.io_apic_address = (uint32_t *)(uint64_t)io_apic_address;
                kernel->interrupts.apic.gsi_base = *(uint32_t *)(apic_entry + 8);
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
