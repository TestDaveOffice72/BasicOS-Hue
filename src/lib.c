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
