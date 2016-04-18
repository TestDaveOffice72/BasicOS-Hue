#include "lib.h"
#include "kernel.h"

#define PUSHALL_KAPI "push rax; push rcx; push rdx; push r8; push r9; push r10; push r11;"
#define POPALL_KAPI "pop r11; pop r10; pop r9; pop r8; pop rdx; pop rcx; pop rax;"

NAKED void
unknown_handler()
{
    __asm__(".intel_syntax;"
            "1:"
            "mov ecx, 0ffc1272dh;"
            "call fill_screen;"
            "hlt;"
            "jmp 1b;");
}

NAKED void
unknown_software_handler()
{
    __asm__(".intel_syntax;"
            "1:"
            "mov ecx, 0006a44h;"
            "call fill_screen;"
            "hlt;"
            "jmp 1b;");
}

NAKED void
unknown_irq_handler()
{
    __asm__(".intel_syntax;"
            "1:"
            "mov ecx, 06a0044h;"
            "call fill_screen;"
            "hlt;"
            "jmp 1b;");
}

NAKED void
int13_handler()
{
    __asm__(".intel_syntax;"
            "1:"
            "mov ecx, 0ffddbb00h;"
            "call fill_screen;"
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
    while(k == 0)
        inb(0x60, k);
    fill_screen(0xff880000 | k);
    for(int i = 0; i < 100000; i++);

    uint8_t a;
    inb(0x61, a);
    a |= 0x82;
    outb(0x61, a);
    a &= 0x7f;
    outb(0x61, a);
    cpu_write_msr(0x80B, 0);
}
