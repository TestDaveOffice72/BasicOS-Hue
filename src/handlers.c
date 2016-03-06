#include "lib.h"

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
unknown_ioapic_handler()
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
