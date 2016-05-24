#include <efi.h>
#include <efiprot.h>

#include "kernel.h"
#include "uefi.h"


struct kernel kernel;
struct kernel *global_kernel;

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
    global_kernel = &kernel;

    serial_print("trying allocate memory\n");
    void *p1 = kmalloc(0x10);
    serial_print_hex(p1);
    serial_print("address received\n");
    void *p2 = kmalloc(0x1002);
    serial_print_hex(p2);
    serial_print("address received\n");
    void *p3 = kmalloc(0x1);
    serial_print_hex(p3);
    serial_print("address received\n");
    kfree(p2);
    p2 = kmalloc(0x1001);
    serial_print_hex(p2);
    serial_print("address received\n");

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
