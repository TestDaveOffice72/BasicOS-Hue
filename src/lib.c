#include "lib.h"

KAPI int kmemcmp(const void *d1, const void *d2, uint64_t len) {
    const uint8_t *d1_ = d1, *d2_ = d2;
    for(uint64_t i = 0; i < len; i += 1, d1_++, d2_++){
        if(*d1_ != *d2_) return *d1_ < *d2_ ? -1 : 1;
    }
    return 0;
}

KAPI void kmemcpy(void *dest, const void *src, uint64_t len) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    for(uint64_t i = 0; i < len; i++, d++, s++) {
        *d = *s;
    }
}

void memset(void *dest, uint8_t e, uint64_t len) {
    uint8_t *d = dest;
    for(uint64_t i = 0; i < len; i++, d++) {
        *d = e;
    }
}

KAPI void kmemset(void *dest, uint8_t e, uint64_t len) {
    memset(dest, e, len);
}

KAPI uint64_t kstrlen(const char *data) {
    uint64_t r;
    for(r = 0; *data != 0; data++, r++);
    return r;
}

KAPI INLINE uint8_t
port_inb(uint16_t port)
{
    uint8_t data;
    __asm__ volatile("inb %w1,%b0" : "=a" (data) : "d"(port));
    return data;
}

KAPI INLINE uint8_t
port_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %b0,%w1" : : "a" (value), "d"(port));
    return value;
}
