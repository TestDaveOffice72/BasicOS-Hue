#include "lib.h"

NAKED void
unknown_handler()
{
    __asm__("call green_screen; hlt");
}

NAKED void
int8_handler()
{
    __asm__("call red_screen; hlt");
}

NAKED void
apic_handler()
{
    __asm__("call blue_screen; hlt");
}
