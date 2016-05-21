#pragma once
#include "lib.h"
#include <efi.h>


struct cpu {
    bool has_apic;
    bool has_x2apic;
    bool has_msr;
    bool has_ia32_efer;
    bool has_sse;
    bool has_sse2;
    bool has_sse3;
    bool has_ssse3;
};

KAPI EFI_STATUS init_cpu(struct cpu *cpu);
KAPI uint64_t cpu_read_msr(uint32_t msr /*rcx*/);
KAPI void cpu_write_msr(uint32_t msr /*rcx*/, uint64_t data/*rdx*/);


KAPI inline uint64_t
read_cr0()
{
    uint64_t value;
    __asm__("movq %%cr0, %0" : "=r"(value));
    return value;
}

KAPI inline void
write_cr0(uint64_t value)
{
    __asm__("movq %0, %%cr0" :: "r"(value));
}

KAPI inline uint64_t
read_cr3()
{
    uint64_t value;
    __asm__("movq %%cr3, %0" : "=r"(value));
    return value;
}

KAPI inline void
write_cr3(uint64_t value)
{
    __asm__("movq %0, %%cr3" :: "r"(value));
}

KAPI inline uint64_t
read_cr4()
{
    uint64_t value;
    __asm__("movq %%cr4, %0" : "=r"(value));
    return value;
}

KAPI inline void
write_cr4(uint64_t value)
{
    __asm__("movq %0, %%cr4" :: "r"(value));
}
