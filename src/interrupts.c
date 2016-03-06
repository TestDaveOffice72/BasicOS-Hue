/*
 * The strategy for interrupts in this kernel is thus:
 *     * We will be using interrupts from 255 backwards for hardware interrupts delivered by APIC.
 *     * Interrupts 32 through 128 will be used as software interrupts, which gives us ~100
 *     interrupts; we do not use DOS-like interrupt model where function is selected through values
 *     in registers in order to:
 *          1. Reduce number of instructions necessary for common syscalls; and
 *          2. Have less clobbered registers.
 *     * The 129th interrupt is for misc software interrupts: this is where rarely used interrupts
 *     go and these use DOS-like model for selecting exact function.
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

// FIXME
KAPI EFI_STATUS init_apic();

KAPI void
ioapic_redirect_irq(uint8_t idx, uint8_t into) {
    // FIXME:
    // Set the ID of local APIC module which will be handling the interrupts.
    // InitializeLib(boot_state.image_handle, boot_state.system_table);
    // Print(L"ACPI signature %d\n", acpi_table->Revision);
    // uint32_t selector = 0x10 + idx * 2 + 1;
    // uint32_t *data_ptr = interrupt_state.ioapic_base + 1;
    // *interrupt_state.ioapic_base = 1;
    // Print(L"IOAPIC base: %x\n", interrupt_state.ioapic_base);

    // uint32_t ioapic_data = *data_ptr;
    // *data_ptr = (ioapic_data & 0x00ffFFFF) | (apic_id << 24);
    // // Set the information about the IRQ
    // *interrupt_state.ioapic_base = selector - 1;
    // ioapic_data = *data_ptr;
    // ioapic_data &= ~0x1ffff;
    // // The IRQ this is mapped into; from 0x10 to 0xfe.
    // ioapic_data |= into & 0xff;
    // ioapic_data |= 0x7 << 8; // fixed delivery mode
    // ioapic_data |= 1 << 11; // physical destination mode
    // *data_ptr = ioapic_data;
}


KAPI EFI_STATUS
init_interrupts()
{
    EFI_STATUS status = init_apic();
    if(EFI_ERROR(status)) return status;

    struct IDT idt = {0};
    __asm__("sidt %0":"=m"(idt));
    interrupt_state.idt_limit = idt.limit;
    interrupt_state.idt_address = (struct idt_descriptor *)idt.offset;

    for(int i = 0; i < 24; i += 1) {
        ioapic_redirect_irq(i, 231 + i);
    }

    struct idt_descriptor unknown_descriptor = {
        .offset_1 = (uint64_t)unknown_handler & 0xFFFF,
        .offset_2 = (((uint64_t)unknown_handler) >> 16) & 0xFFFF,
        .offset_3 = (((uint64_t)unknown_handler) >> 32),
        .selector = 0x28,
        .reserved = 0,
        .flags = 0x8E00
    };
    for(int i = 32; i < 231; i += 1) {
        interrupt_state.idt_address[i] = unknown_descriptor;
    }

    struct idt_descriptor apic_descriptor = {
        .offset_1 = (uint64_t)apic_handler & 0xFFFF,
        .offset_2 = (((uint64_t)apic_handler) >> 16) & 0xFFFF,
        .offset_3 = (((uint64_t)apic_handler) >> 32),
        .selector = 0x28,
        .reserved = 0,
        .flags = 0x8E00
    };
    for(int i = 231; i < 256; i += 1) {
        interrupt_state.idt_address[i] = apic_descriptor;
    }
    // FIXME: we currently use int8 handler for all exception interrupts.
    for(int i = 0; i < 20; i += 1) {
        if(i == 9) continue;
        interrupt_state.idt_address[i].offset_1 = (uint64_t)int8_handler & 0xFFFF;
        interrupt_state.idt_address[i].offset_2 = (((uint64_t)int8_handler) >> 16) & 0xFFFF;
        interrupt_state.idt_address[i].offset_3 = (((uint64_t)int8_handler) >> 32);
        interrupt_state.idt_address[i].selector = 0x28;
        interrupt_state.idt_address[i].flags = 0x8E00;
    }

    return EFI_SUCCESS;
}

inline void
outb(uint8_t port, uint8_t data)
{
    __asm__("out %0, %1" :: "a"(data), "i"(port));
}

inline void
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
    if(apic_info == NULL) {
        return EFI_NOT_FOUND;
    }
    uint32_t flags = *((uint32_t*)(apic_info + 1) + 1);
    uint32_t apic_data_len = apic_info->length - sizeof(struct XSDTHeader) - 8;
    uint8_t *apic_entry = ((uint8_t *)(apic_info + 1)) + 8;

    while(apic_data_len > 0) {
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
                interrupt_state.apic_info.io_apic_address = *(uint32_t *)(apic_entry + 4);
                interrupt_state.apic_info.gsi_base = *(uint32_t *)(apic_entry + 8);
                break;
            }

            // we do not care about these.
            case 2: // namely, we probably should handle ISOs, but this is reaaly out of scope for
                    // university kernel.
            default: break;
        }
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

    // FIXME: need to retrieve proper address from ACPI
    // interrupt_state.ioapic_base = (uint32_t *)0xfec00000;
    return EFI_SUCCESS;
}
