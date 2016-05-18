#pragma once
#include "lib.h"
#include "uefi.h"

struct serial {};

KAPI EFI_STATUS
init_serial(const struct uefi *uefi, struct serial *serial);
KAPI uint64_t
serial_port_write(uint8_t *buffer, uint64_t size);

KAPI inline void
serial_print(const char *print) {
    serial_port_write((uint8_t *)print, kstrlen(print));
}

KAPI inline void
serial_print_int(uint64_t n) {
    char buf[24] = {0}, *bp = buf + 24;
    do {
        bp--;
        *bp = '0' + n % 10;
        n /= 10;
    } while (n != 0);
    serial_port_write((uint8_t *)bp, buf - bp + 24);
}

KAPI inline void
serial_print_hex(uint64_t n) {
    char buf[16], *bp = buf + 16;
    for(int i = 0; i < 16; i++) buf[i] = '0';
    do {
        bp--;
        uint8_t mod = n % 16;
        if(mod < 10) {
            *bp = '0' + mod;
        } else {
            *bp = 'A' - 10 + mod;
        }
        n /= 16;
    } while (n != 0);
    serial_port_write((uint8_t *)buf, 16);
}
