#include <efi.h>
#include <efiprot.h>

#include "kernel.h"
#include "uefi.h"

KAPI void fill_screen(const struct graphics *gs, uint32_t rgb);

struct kernel kernel;

EFI_STATUS
efi_main (EFI_HANDLE ih, EFI_SYSTEM_TABLE *st)
{
    kernel.uefi.image_handle = ih;
    kernel.uefi.system_table = st;
    EFI_STATUS status;

    status = init_serial(&kernel.uefi, &kernel.serial);
    ASSERT_EFI_STATUS(status);
    serial_print("Kernel booting\n");

    serial_print("CPU…\n");
    status = init_cpu(&kernel.cpu);
    ASSERT_EFI_STATUS(status);

    serial_print("Graphics…\n");
    // Initialize graphics
    status = init_graphics(&kernel.uefi, &kernel.graphics);
    ASSERT_EFI_STATUS(status);

    serial_print("Interrupts…\n");
    // Init interrupts
    status = init_interrupts(&kernel);
    ASSERT_EFI_STATUS(status);

    serial_print("Memory…\n");
    // // Initialize memory management subsystem and exit
    status = init_memory(&kernel);
    ASSERT_EFI_STATUS(status);

    serial_print("Drawing the thing\n");
    for(uint8_t o = 0; o <= 100; o += 1) {
        for(int x = 0; x < 1920; x += 1) {
            for(int y = 0; y < 360; y += 1) {
                uint32_t r = (0xfd * o / 100 ) << 16;
                uint32_t g = (0xb9 * o / 100 ) << 8;
                uint32_t b = (0x13 * o / 100 );
                set_pixel(&kernel.graphics, x, y, r | g | b);
            }
        }
        for(int x = 0; x < 1920; x += 1) {
            for(int y = 360; y < 720; y += 1) {
                uint32_t g = (0x6a * o / 100) << 8;
                uint32_t b = (0x44 * o / 100);
                set_pixel(&kernel.graphics, x, y, g | b);
            }
        }
        for(int x = 0; x < 1920; x += 1) {
            for(int y = 720; y < 1080; y += 1) {
                uint32_t r = (0xc1 * o / 100) << 16;
                uint32_t g = (0x27 * o / 100) << 8;
                uint32_t b = (0x2d * o / 100);
                set_pixel(&kernel.graphics, x, y, r | g | b);
            }
        }
    }

    serial_print("Reached end of efi_main!\n");
    // Once we’re done we poweroff the machine.
    st->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    serial_print("Shutdown didn’t work!!!\n");
    for(;;) __asm__ volatile("hlt");
}
