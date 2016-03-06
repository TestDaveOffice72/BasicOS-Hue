#pragma once
#include <efi.h>
#include <inttypes.h>
#include <stdbool.h>

#define KAPI __attribute__((ms_abi))
#define NAKED __attribute__((naked))

KAPI int kmemcmp(const void *d1, const void *d2, uint64_t len);
