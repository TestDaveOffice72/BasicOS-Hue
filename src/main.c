#include <efi.h>
#include <efilib.h>
#include <efiprot.h>

#include "kernel.h"

struct boot_state boot_state;

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    InitializeLib(ImageHandle, SystemTable);

    // Initialize graphics
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;
    EFI_GUID graphics_proto = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    status = uefi_call_wrapper(SystemTable->BootServices->LocateProtocol, 3,
                               &graphics_proto, NULL, &graphics);
    ASSERT_EFI_STATUS(status, L"LocateProtocol Graphics");
    status = init_graphics(graphics);
    if(status != EFI_SUCCESS) return status;

    // Figure out the size of memory map.
    boot_state.memory_map = LibMemoryMap(&boot_state.memory_map_size, &boot_state.map_key,
                                         &boot_state.descriptor_size,
                                         &boot_state.descriptor_version);

    uefi_call_wrapper(SystemTable->BootServices->ExitBootServices, 2,
                      ImageHandle, boot_state.map_key);
    uefi_call_wrapper(SystemTable->RuntimeServices->SetVirtualAddressMap, 4,
                      boot_state.memory_map_size, boot_state.descriptor_size,
                      boot_state.descriptor_version, boot_state.memory_map);

    // Init interrupts
    status = init_interrupts();
    ASSERT_EFI_STATUS(status, L"init interrupts");

    // Some work, blends in the lithuanian flag
    for(uint8_t o = 0; o <= 100; o += 1) {
        for(int x = 0; x < 1920; x += 1) {
            for(int y = 0; y < 360; y += 1) {
                uint32_t r = (0xfd * o / 100 ) << 16;
                uint32_t g = (0xb9 * o / 100 ) << 8;
                uint32_t b = (0x13 * o / 100 );
                set_pixel(x, y, r | g | b);
            }
        }
        for(int x = 0; x < 1920; x += 1) {
            for(int y = 360; y < 720; y += 1) {
                uint32_t g = (0x6a * o / 100) << 8;
                uint32_t b = (0x44 * o / 100);
                set_pixel(x, y, g | b);
            }
        }
        for(int x = 0; x < 1920; x += 1) {
            for(int y = 720; y < 1080; y += 1) {
                uint32_t r = (0xc1 * o / 100) << 16;
                uint32_t g = (0x27 * o / 100) << 8;
                uint32_t b = (0x2d * o / 100);
                set_pixel(x, y, r | g | b);
            }
        }
    }

    // Once we’re done we poweroff the machine.
    uefi_call_wrapper(SystemTable->RuntimeServices->ResetSystem, 4,
                      EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    for(;;) __asm__("hlt");
}
