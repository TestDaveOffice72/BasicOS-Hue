#pragma once
#include "lib.h"
#include "uefi.h"

struct serial {};

KAPI EFI_STATUS
init_serial(const struct uefi *uefi, struct serial *serial);
KAPI uint64_t
serial_port_write(uint8_t *buffer, uint64_t size);

KAPI INLINE void
serial_print(const char *print);

KAPI void
serial_print_int(uint64_t n);

KAPI void
serial_print_hex(uint64_t n);
