#include <efi.h>
#include <efiprot.h>

#include "kernel.h"
#include "uefi.h"

KAPI void fill_screen(const struct graphics *gs, uint32_t rgb);

struct kernel kernel;

extern uint8_t _binary_bin_init_com_start;
extern uint8_t _binary_bin_init_com_end;

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

    serial_print("Starting init\n");
    status = start_init(&kernel,
                        (uint8_t *)&_binary_bin_init_com_start,
                        (uint8_t *)&_binary_bin_init_com_end);
    serial_print("Reached end of efi_main!\n");
    DEBUG_HALT;
    // Once we’re done we poweroff the machine.
    // st->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    // serial_print("Shutdown didn’t work!!!\n");
    // DEBUG_HALT;
}
