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
