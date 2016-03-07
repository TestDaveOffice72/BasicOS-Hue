#pragma once
#include <efi.h>
#include <inttypes.h>
#include <stdbool.h>

#define KAPI __attribute__((ms_abi))
#define NAKED __attribute__((naked))

#define outb(p, d) {__asm__ volatile("outb %0, %1" :: "a"((uint8_t)d), "i"(p));}
#define inb(p, d) {__asm__ volatile("inb %1, %0" : "=a"(d) : "i"((uint8_t)p));}

KAPI int kmemcmp(const void *d1, const void *d2, uint64_t len);
KAPI void kmemcpy(void *dest, const void *src, uint64_t len);
