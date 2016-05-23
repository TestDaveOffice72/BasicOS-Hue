#pragma once
#include "lib.h"

#include "acpi.h"
#include "uefi.h"
#include "cpu.h"
#include "interrupts.h"
#include "graphics.h"
#include "memory.h"
#include "serial.h"
#include "process.h"


struct kernel {
    struct memory memory;
    struct graphics graphics;
    struct cpu cpu;
    struct interrupts interrupts;
    struct uefi uefi;
    struct serial serial;
    struct processes processes;
};

extern struct kernel *global_kernel;
