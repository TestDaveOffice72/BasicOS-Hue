#include "kernel.h"

struct graphics_info graphics_info;

KAPI EFI_STATUS
select_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics, UINT32 *mode);

KAPI EFI_STATUS
init_graphics(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics)
{
    UINT32 new_mode = graphics->Mode->Mode;
    EFI_STATUS status = select_mode(graphics, &new_mode);
    ASSERT_EFI_STATUS(status);

    status = graphics->SetMode(graphics, new_mode);
    ASSERT_EFI_STATUS(status);
    graphics_info.protocol = graphics;
    graphics_info.buffer_base = (void*)graphics->Mode->FrameBufferBase;
    graphics_info.buffer_size = graphics->Mode->FrameBufferSize;

    return EFI_SUCCESS;
}

KAPI EFI_STATUS
select_mode(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics, UINT32 *mode)
{
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION most_appropriate_info;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    UINTN size;

    // Initialize info of current mode
    EFI_STATUS status = graphics->QueryMode(graphics, *mode, &size, &info);
    ASSERT_EFI_STATUS(status);
    kmemcpy(&most_appropriate_info, info, sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

    // Look for a better mode
    for(UINT32 i = 0; i < graphics->Mode->MaxMode; i += 1) {
        // Find out the parameters of the mode we’re looking at
        EFI_STATUS status = graphics->QueryMode(graphics, i, &size, &info);
        ASSERT_EFI_STATUS(status);
        // We only accept RGB or BGR 8 bit colorspaces.
        if(info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor &&
           info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor) {
            continue;
        }
        // If either of resolutions exceed appropriate w/h we cannot use the mode.
        if(info->HorizontalResolution > GRAPHICS_MOST_APPROPRIATE_W ||
           info->VerticalResolution > GRAPHICS_MOST_APPROPRIATE_H) {
            continue;
        }
        // Obviously the best mode!
        if(info->VerticalResolution == GRAPHICS_MOST_APPROPRIATE_H &&
           info->HorizontalResolution == GRAPHICS_MOST_APPROPRIATE_W) {
            kmemcpy(&most_appropriate_info, info, sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
            *mode = i;
            break;
        }
        // Otherwise we have an arbitrary preferece to get as much vertical resolution as possible.
        if(info->VerticalResolution > most_appropriate_info.VerticalResolution) {
            kmemcpy(&most_appropriate_info, info, sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
            *mode = i;
        }
    }
    kmemcpy(&graphics_info.output_mode, &most_appropriate_info,
            sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
    return EFI_SUCCESS;
}

void set_pixel(int w, int h, uint32_t rgb) {
    w *= 4;
    h *= 4;
    int32_t *addr = graphics_info.buffer_base + w + h * graphics_info.output_mode.PixelsPerScanLine;
    *addr = rgb;
}

KAPI
void fill_screen(uint32_t rgb) {
    rgb |= 0xff000000;
    for(int x = 0; x < graphics_info.output_mode.HorizontalResolution; x += 1) {
        for(int y = 0; y < graphics_info.output_mode.VerticalResolution; y += 1) {
            set_pixel(x, y, rgb);
        }
    }
}
