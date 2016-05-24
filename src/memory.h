#pragma once
#include "lib.h"
#include <efi.h>

#define FOR_EACH_MEMORY_DESCRIPTOR(kernel, desc) \
    EFI_MEMORY_DESCRIPTOR *desc = kernel->uefi.boot_memmap.descrs;\
    uint8_t **descr_offsetter = (uint8_t **)&(desc);\
    uint64_t offset = kernel->uefi.boot_memmap.descr_size;\
    for(uint64_t __i = 0; \
        __i < kernel->uefi.boot_memmap.entries;\
        __i += 1, *descr_offsetter += offset)

struct memory {
    uint64_t              *first_free_page;
    uint64_t              *pml4_table;
    uint64_t              max_addr;
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
KAPI void set_paging(bool on);
/// Allocate a single page. Returns NULL if there’s no more pages to allocate.
KAPI void *allocate_page(struct kernel *k);
/// Deallocate a single page.
KAPI void deallocate_page(struct kernel *k, void *p);

/// Map a single page to a specified address.
KAPI void map_page(struct kernel *k, uint64_t address, uint8_t level, uint64_t phys_addr);


KAPI void kfree(void *ptr);
KAPI void *kmalloc(uint64_t nbytes);
