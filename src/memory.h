#pragma once
#include "lib.h"
#include <efi.h>

struct memory {
    EFI_MEMORY_DESCRIPTOR *memory_map;
    uint64_t              memory_map_size;
    uint64_t              map_key;
    uint64_t              descriptor_size;
    uint32_t              descriptor_version;
};

typedef enum {
    _EfiReservedMemoryType,
    _EfiLoaderCode,
    _EfiLoaderData,
    _EfiBootServicesCode,
    _EfiBootServicesData,
    _EfiRuntimeServicesCode,
    _EfiRuntimeServicesData,
    _EfiConventionalMemory,
    _EfiUnusableMemory,
    _EfiACPIReclaimMemory,
    _EfiACPIMemoryNVS,
    _EfiMemoryMappedIO,
    _EfiMemoryMappedIOPortSpace,
    _EfiPalCode,
    _EfiPersistentMemory,
    _EfiMaxMemoryType
} _EFI_MEMORY_TYPE;

// Initializes memory and exits EFI boot services
struct kernel;
KAPI EFI_STATUS
init_memory(struct kernel *kernel);
