#include <efi.h>
#include <efiprot.h>

#include "kernel.h"
#include "uefi.h"

struct boot_state boot_state;

EFI_STATUS
efi_main (EFI_HANDLE ih, EFI_SYSTEM_TABLE *st)
{
    image_handle = ih;
    system_table = st;
    EFI_STATUS status;

    status = init_cpu();
    ASSERT_EFI_STATUS(status);

    // Initialize graphics
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;
    EFI_GUID graphics_proto = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    status = system_table->BootServices->LocateProtocol(&graphics_proto, NULL, (void **)&graphics);
    ASSERT_EFI_STATUS(status);
    status = init_graphics(graphics);
    ASSERT_EFI_STATUS(status);

    // Init interrupts
    status = init_interrupts();
    ASSERT_EFI_STATUS(status);

    // Figure out the size of memory map.
    // boot_state.memory_map = LibMemoryMap(&boot_state.memory_map_size, &boot_state.map_key,
    //                                      &boot_state.descriptor_size,
    //                                      &boot_state.descriptor_version);

    // SystemTable->BootServices->ExitBootServices(ImageHandle, boot_state.map_key);
    // SystemTable->RuntimeServices->SetVirtualAddressMap(boot_state.memory_map_size,
    //                                                    boot_state.descriptor_size,
    //                                                    boot_state.descriptor_version,
    //                                                    boot_state.memory_map);

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
    system_table->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    for(;;) __asm__ volatile("hlt");
}
