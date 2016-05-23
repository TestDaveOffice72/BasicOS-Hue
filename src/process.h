#pragma once

#include "kernel.h"
#include "lib.h"

struct process {
    uint64_t ip;
    uint64_t sp;

};

// These all forked thus share the same memory.
struct processes {
    // Up to 16 forks for now
    struct process ps[16];
    uint8_t current_process;
    // bitmask
    uint16_t running_processes;
};

// Initialize and start the init process, must already be loaded.
KAPI EFI_STATUS start_init(struct kernel *, uint8_t *init_com_start, uint8_t *init_com_end);
KAPI void switch_to(struct kernel *k, uint8_t process);
