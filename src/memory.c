#include "memory.h"
#include "serial.h"
#include "kernel.h"
#include "uefi.h"

KAPI EFI_STATUS
init_memory(struct kernel *kernel) {
    EFI_STATUS status;
    status = efi_memory_map(&kernel->uefi, &kernel->uefi.boot_memmap);
    ASSERT_EFI_STATUS(status);
    serial_print("EFI memory map has ");
    serial_print_int(kernel->uefi.boot_memmap.entries);
    serial_print(" entries (type pstart~vstart, numpages, attrs):\n");

    EFI_MEMORY_DESCRIPTOR *descr = kernel->uefi.boot_memmap.descrs;
    uint8_t **descr_offsetter = (uint8_t **)&descr;
    uint64_t offset = kernel->uefi.boot_memmap.descr_size;
    for(uint64_t i = 0; i < kernel->uefi.boot_memmap.entries; i += 1, *descr_offsetter += offset) {
        switch(descr->Type){
            case _EfiReservedMemoryType: serial_print("R"); break;
            case _EfiLoaderCode: serial_print("L"); break;
            case _EfiLoaderData: serial_print("l"); break;
            case _EfiBootServicesCode: serial_print("B"); break;
            case _EfiBootServicesData: serial_print("b"); break;
            case _EfiRuntimeServicesCode: serial_print("R"); break;
            case _EfiRuntimeServicesData: serial_print("r"); break;
            case _EfiConventionalMemory: serial_print("C"); break;
            case _EfiUnusableMemory: serial_print("U"); break;
            case _EfiACPIReclaimMemory: serial_print("A"); break;
            case _EfiACPIMemoryNVS: serial_print("a"); break;
            case _EfiMemoryMappedIO: serial_print("M"); break;
            case _EfiMemoryMappedIOPortSpace: serial_print("m"); break;
            case _EfiPalCode: serial_print("p"); break;
            case _EfiPersistentMemory: serial_print("P"); break;
            case _EfiMaxMemoryType: serial_print("X"); break;
            default: serial_print("!"); break;
        }
        serial_print(" 0x");
        serial_print_hex(descr->PhysicalStart);
        serial_print("~0x");
        serial_print_hex(descr->VirtualStart);
        serial_print("\t");
        serial_print_int(descr->NumberOfPages);
        serial_print("\t");
#define CHECK_ATTR(ty) ({ if(descr->Attribute | EFI_MEMORY_ ## ty) { serial_print(" " #ty); } })
        CHECK_ATTR(UC);
        CHECK_ATTR(WC);
        CHECK_ATTR(WT);
        CHECK_ATTR(WB);
        CHECK_ATTR(UCE);
        CHECK_ATTR(WP);
        CHECK_ATTR(RP);
        CHECK_ATTR(XP);
        CHECK_ATTR(RUNTIME);
        serial_print("\n");
        // Within the kernel we use identity mapping.
        descr->VirtualStart = descr->PhysicalStart;
    }

    status = kernel->uefi.system_table->BootServices->ExitBootServices(
        kernel->uefi.image_handle,
        kernel->uefi.boot_memmap.map_key
    );

    serial_print("Boot services done\n");
    ASSERT_EFI_STATUS(status);
    // TODO: should setup our own memory map and virtual mapping now

    status = kernel->uefi.system_table->RuntimeServices->SetVirtualAddressMap(
        kernel->uefi.boot_memmap.entries * kernel->uefi.boot_memmap.descr_size,
        kernel->uefi.boot_memmap.descr_size,
        kernel->uefi.boot_memmap.descr_version,
        kernel->uefi.boot_memmap.descrs
    );
    ASSERT_EFI_STATUS(status);

    return EFI_SUCCESS;
}

KAPI uint64_t
read_cr3()
{
    uint64_t value;
    __asm__("movq %%cr3, %0" : "=r"(value));
    return value;
}

KAPI void
write_cr3(uint64_t value)
{
    __asm__("movq %0, %%cr3" :: "r"(value));
}
