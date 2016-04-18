#include "cpu.h"

const uint32_t HAS_APIC = 1 << 9;
const uint32_t HAS_X2APIC = 1 << 21;
const uint32_t HAS_MSR = 1 << 5;

KAPI EFI_STATUS
init_cpu(struct cpu *cpu)
{
    uint32_t b, c, d, a = 1;
    __asm__("cpuid" : "=d"(d), "=b"(b), "=c"(c), "+a"(a));
    cpu->has_apic = (d & HAS_APIC) != 0;
    cpu->has_x2apic = (c & HAS_X2APIC) != 0;
    cpu->has_msr = (d & HAS_MSR) != 0;
    return EFI_SUCCESS;
}

KAPI uint64_t
cpu_read_msr(uint32_t msr) {
    uint32_t a, d;
    __asm__ volatile("rdmsr" : "=a"(a), "=d"(d) : "c"(msr));
    return ((uint64_t)d) << 32 | a;
}

KAPI void
cpu_write_msr(uint32_t msr, uint64_t data) {
    __asm__ volatile("wrmsr;"::"c"(msr), "a"((uint32_t)data), "d"((uint32_t)(data>>32)));
}
