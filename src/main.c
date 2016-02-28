#include <efi.h>
#include <efilib.h>

struct boot_state {
    UINTN                 memory_map_size;
    EFI_MEMORY_DESCRIPTOR *memory_map;
    UINTN                 map_key;
    UINTN                 descriptor_size;
    UINT32                descriptor_version;
};

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    uefi_call_wrapper(SystemTable->ConOut->ClearScreen, 1, SystemTable->ConOut);
    uefi_call_wrapper(SystemTable->ConOut->OutputString, 2, SystemTable->ConOut,
                      L"HueHueHueHueHue is here!\r\n");

    struct boot_state state;
    // Figure out the size of memory map.
    state.memory_map = LibMemoryMap(&state.memory_map_size, &state.map_key, &state.descriptor_size,
                                    &state.descriptor_version);
    uefi_call_wrapper(SystemTable->ConOut->OutputString, 2, SystemTable->ConOut,
                      L"Exiting BootServices\r\n");
    uefi_call_wrapper(SystemTable->BootServices->ExitBootServices, 2, ImageHandle, state.map_key);

    uefi_call_wrapper(SystemTable->RuntimeServices->SetVirtualAddressMap, 4,
                      state.memory_map_size, state.descriptor_size, state.descriptor_version,
                      state.memory_map);

    for(int i = 0; i < 1000000000; i++); // some work to do


    // Once we’re done we poweroff the machine.
    uefi_call_wrapper(SystemTable->RuntimeServices->ResetSystem, 4,
                      EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    return EFI_SUCCESS;
}
