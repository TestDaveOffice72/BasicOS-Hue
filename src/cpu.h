#pragma once
#include "lib.h"
#include <efi.h>

struct cpu {
    bool has_apic;
    bool has_x2apic;
    bool has_msr;
};

KAPI EFI_STATUS init_cpu(struct cpu *cpu);
KAPI uint64_t cpu_read_msr(uint32_t msr /*rcx*/);
KAPI void cpu_write_msr(uint32_t msr /*rcx*/, uint64_t data/*rdx*/);
