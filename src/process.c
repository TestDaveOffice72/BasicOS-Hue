#include "kernel.h"
#include "memory.h"
#include "process.h"

KAPI EFI_STATUS start_init(struct kernel *k, uint8_t *init_com_start, uint8_t *init_com_end){
    // We’re cheating somewhat here. Since the only two things we need to implement are fork and
    // malloc, we have a lot of simplifying circumstances going for us:
    //
    // 1. We only ever gonna need 1 set of pages at a certain address (0x100000000 for example) for
    // program code and data;
    // 2. Forking reuses the same memory (yay!);
    // 3. Thus we can avoid making proper userspace and can run the program in kernel-space;
    // 4. This means we can just reuse our page allocator for user-space malloc too.

    // So, first of all, create a page at the location we’ve decided to keep our kernel at. That’s
    // 0x100000000.
    uint64_t size = init_com_end - init_com_start;
    for(uint64_t s = 0; s < size; s += 0x1000) {
        void *page = allocate_page(k);
        if(page == NULL) return EFI_OUT_OF_RESOURCES;
        map_page(k, 0x100000000ull + s, 0, (uint64_t)page);
    }
    kmemcpy((void *)0x100000000ull, (void *)init_com_start, size);
    // At this point the executable image is loaded in, record various information about it in the
    // process table.
    struct process p = {.ip = 0x100000000, .sp = 0x0};
    k->processes.ps[0] = p;
    k->processes.running_processes = 1;
    k->processes.current_process = 0;
    // Remember where the kernel struct lives here
    serial_print("Switching to init process\n");
    switch_to(k, 0);
    serial_print("Switching to init process failed");
    DEBUG_HALT;
}

KAPI void
switch_to(struct kernel *k, uint8_t process)
{
    // It is undefined behaviour for the program to return. The only thing we need to do
    // is to jump to its code after adjusting the stack a bit.
    struct process *p = &k->processes.ps[process];
    __asm__("mov %1, %%rsp; jmp *%0;" :: "r"(p->ip), "r"(p->sp) : "%rax");
    DEBUG_HALT;
}
