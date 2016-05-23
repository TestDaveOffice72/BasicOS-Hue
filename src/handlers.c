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
            "cli;"
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

NAKED void
int34_handler()
{
    __asm__(".intel_syntax;"
            PUSHALL_KAPI
            "mov rdx, rcx;"
            "mov rcx, rax;"
            "call int34_inner;"
            POPALL_KAPI
            "iretq");
}

KAPI void
int34_inner(uint8_t *msg, uint64_t len)
{
    serial_port_write(msg, len);
}


NAKED void
int35_handler()
{
    __asm__(".intel_syntax;"
            PUSHALL_KAPI
            "call int35_inner;"
            POPALL_KAPI
            "iretq");
}

KAPI void
int35_inner(uint64_t new_ip)
{
    uint64_t available_slot = __builtin_ffs(~global_kernel->processes.running_processes) - 1;
    global_kernel->processes.running_processes |= 1 << available_slot;
    struct process p = { .ip = new_ip, .sp = 0x0 };
    global_kernel->processes.ps[available_slot] = p;
}

NAKED void
int36_handler()
{
    __asm__(".intel_syntax;"
            PUSHALL_KAPI
            "mov rcx, rsp;"
            "call int36_inner;"
            POPALL_KAPI
            "iretq");
}

KAPI void
int36_inner(uint64_t *rsp)
{
    uint64_t *current_ip = rsp + 7;
    uint64_t *current_sp = rsp + 10;
    struct process *p = &global_kernel->processes.ps[global_kernel->processes.current_process];
    p->ip = *current_ip;
    p->sp = *current_sp;
    uint64_t next_current = (global_kernel->processes.current_process + 1) % 16;
    while(true) {
        if((global_kernel->processes.running_processes & (1 << next_current)) != 0) {
            p = &global_kernel->processes.ps[next_current];
            serial_print("Switching to process ");
            serial_print_int(next_current);
            serial_print(" with IP = ");
            serial_print_hex(p->ip);
            serial_print(" SP = ");
            serial_print_hex(p->sp);
            serial_print("\n");
            global_kernel->processes.current_process = next_current;
            *current_ip = p->ip;
            *current_sp = p->sp;
            return;
        }
        next_current = (next_current + 1) % 16;
    }
}

NAKED void
int37_handler()
{
    __asm__(".intel_syntax;"
            "push rcx; push rdx; push r8; push r9; push r10; push r11;"
            "call int37_inner;"
            "pop r11; pop r10; pop r9; pop r8; pop rdx; pop rcx;"
            "iretq");
}

KAPI void *
int37_inner()
{
    return allocate_page(global_kernel);
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

KAPI void
irq1_inner()
{
    uint8_t k = 0;
    while((k = port_inb(0x60)) == 0);
    serial_print("Keypress keycode:");
    serial_print_int(k);
    serial_print("\n");
    uint8_t a = port_inb(0x61);
    a |= 0x82;
    port_outb(0x61, a);
    a &= 0x7f;
    port_outb(0x61, a);
    cpu_write_msr(0x80B, 0);
}
