#pragma once
#include <inttypes.h>
#include <stdbool.h>

#define KAPI __attribute__((ms_abi))
#define NAKED __attribute__((naked))
#define ASSERT_EFI_STATUS(x) {if(EFI_ERROR((x))) { return x; }}
#define DEBUG_HALT for(;;) __asm__ volatile("hlt");

KAPI int kmemcmp(const void *d1, const void *d2, uint64_t len);
KAPI void kmemcpy(void *dest, const void *src, uint64_t len);
KAPI void kmemset(void *dest, uint8_t b, uint64_t len);
KAPI uint64_t kstrlen(const char *d);

KAPI inline uint8_t
port_inb(uint16_t port)
{
    uint8_t data;
    __asm__ volatile("inb %w1,%b0" : "=a" (data) : "d"(port));
    return data;
}

KAPI inline uint8_t
port_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %b0,%w1" : : "a" (value), "d"(port));
    return value;
}
