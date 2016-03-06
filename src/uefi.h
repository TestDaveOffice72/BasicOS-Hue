#pragma once
#include <efi.h>
#include <efiprot.h>

#include "lib.h"

#define EFI_ACPI_TABLE_GUID {0x8868e871,0xe4f1,0x11d3, {0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81}}

extern EFI_SYSTEM_TABLE *system_table;
extern EFI_HANDLE image_handle;

KAPI void *
find_configuration_table(EFI_GUID *guid);
KAPI bool
compare_guid(EFI_GUID *g1, EFI_GUID *g2);
