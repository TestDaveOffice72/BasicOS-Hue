#include "memory.h"
#include "kernel.h"
#include "uefi.h"

KAPI EFI_STATUS
init_memory(struct kernel *kernel) {
    EFI_STATUS status = efi_memory_map(&kernel->uefi, &kernel->uefi.boot_memmap);
    ASSERT_EFI_STATUS(status);
    status = kernel->uefi.system_table->BootServices->ExitBootServices(
        kernel->uefi.image_handle,
        kernel->uefi.boot_memmap.map_key
    );
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
