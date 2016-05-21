#include "lib.h"
#include "kernel.h"

#define PUSHALL_KAPI "push rax; push rcx; push rdx; push r8; push r9; push r10; push r11;"
#define POPALL_KAPI "pop r11; pop r10; pop r9; pop r8; pop rdx; pop rcx; pop rax;"

NAKED void
unknown_handler()
{
    __asm__(".intel_syntax;"
            "mov dx, 03f8h;"
            "mov al, 023h;"
            "out dx, al;"
            "mov al, '?';"
            "out dx, al;"
            "mov al, '?';"
            "out dx, al;"
            "mov al, 0Ah;"
            "out dx, al;"
            "1:"
            "hlt;"
            "jmp 1b;");
}

NAKED void
unknown_software_handler()
{
    __asm__(".intel_syntax;"
            "mov dx, 03f8h;"
            "mov al, 023h;"
            "out dx, al;"
            "mov al, 'S';"
            "out dx, al;"
            "mov al, '?';"
            "out dx, al;"
            "mov al, 0Ah;"
            "out dx, al;"
            "1:"
            "hlt;"
            "jmp 1b;");
}

NAKED void
unknown_irq_handler()
{
    __asm__(".intel_syntax;"
            "mov dx, 03f8h;"
            "mov al, 023h;"
            "out dx, al;"
            "mov al, 'I';"
            "out dx, al;"
            "mov al, '?';"
            "out dx, al;"
            "mov al, 0Ah;"
            "out dx, al;"
            "1:"
            "hlt;"
            "jmp 1b;");
}

NAKED void
df_handler()
{
    __asm__(".intel_syntax;"
            "mov dx, 03f8h;"
            "mov al, 023h;"
            "out dx, al;"
            "mov al, 'D';"
            "out dx, al;"
            "mov al, 'F';"
            "out dx, al;"
            "mov al, 0Ah;"
            "out dx, al;"
            "1:"
            "hlt;"
            "jmp 1b;");
}

NAKED void
gp_handler()
{
    __asm__(".intel_syntax;"
            "mov dx, 03f8h;"
            "mov al, 023h;"
            "out dx, al;"
            "mov al, 'G';"
            "out dx, al;"
            "mov al, 'P';"
            "out dx, al;"
            "mov al, 0Ah;"
            "out dx, al;"
            "1:"
            "hlt;"
            "jmp 1b;");
}

NAKED void
pf_handler()
{
    __asm__(".intel_syntax;"
            "mov dx, 03f8h;"
            "mov al, 023h;"
            "out dx, al;"
            "mov al, 'P';"
            "out dx, al;"
            "mov al, 'F';"
            "out dx, al;"
            "mov al, 0Ah;"
            "out dx, al;"
            "1:"
            "hlt;"
            "jmp 1b;");
}

NAKED void
ud_handler()
{
    __asm__(".intel_syntax;"
            "mov dx, 03f8h;"
            "mov al, 023h;"
            "out dx, al;"
            "mov al, 'U';"
            "out dx, al;"
            "mov al, 'D';"
            "out dx, al;"
            "mov al, 0Ah;"
            "out dx, al;"
            "1:"
            "hlt;"
            "jmp 1b;");
}

NAKED void
int32_handler()
{
    __asm__("iretq");
}

NAKED void
int33_handler()
{
    __asm__("1: hlt; jmp 1b;");
}

__attribute__((naked, ms_abi)) void
irq1_handler(uint32_t a)
{
    __asm__(".intel_syntax;"
            PUSHALL_KAPI
            "call irq1_inner;"
            POPALL_KAPI
            "iretq;");
}

KAPI void fill_screen(uint32_t rgb);

KAPI void
irq1_inner()
{
    uint8_t k = 0;
    while((k = port_inb(0x60)) == 0);
    fill_screen(0xff880000 | k);
    uint8_t a = port_inb(0x61);
    a |= 0x82;
    port_outb(0x61, a);
    a &= 0x7f;
    port_outb(0x61, a);
    cpu_write_msr(0x80B, 0);
}
