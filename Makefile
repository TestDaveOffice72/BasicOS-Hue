ARCH          = x86_64

HEADERS       = src/kernel.h
OBJS          = src/main.o src/graphics.o src/interrupts.o

EFIINC        = /usr/include/efi
EFIINCS       = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
EFI_CRT_OBJS  = /usr/lib/crt0-efi-$(ARCH).o
EFI_LDS       = /usr/lib/elf_$(ARCH)_efi.lds
OVMF          = /usr/share/ovmf/ovmf_x64.bin
QEMU_OPTS     = -enable-kvm -m 64 -device VGA

CFLAGS        = $(EFIINCS) -xc -std=c11 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone \
-Wall -Wno-incompatible-library-redeclaration -O2

ifeq ($(ARCH),x86_64)
  CFLAGS += -DHAVE_USE_MS_ABI
endif

LDFLAGS       = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L /usr/lib $(EFI_CRT_OBJS)

all: image.img

run: image.img
	qemu-system-x86_64 -bios $(OVMF) -drive file=image.img,if=ide $(QEMU_OPTS)

image.img: data.img
	dd if=/dev/zero of=$@ bs=512 count=93750
	parted $@ -s -a minimal mklabel gpt
	parted $@ -s -a minimal mkpart EFI FAT16 2048s 93716s
	parted $@ -s -a minimal toggle 1 boot
	dd if=data.img of=$@ bs=512 count=91669 seek=2048 conv=notrunc

data.img: huehuehuehuehue.efi
	dd if=/dev/zero of=$@ bs=512 count=91669
	mformat -i $@ -h 32 -t 32 -n 64 -c 1
	mcopy -i $@ $< ::/

huehuehuehuehue.so: $(OBJS)
	ld.gold $(LDFLAGS) $(OBJS) -o $@ -lefi -lgnuefi

%.efi: %.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .reloc --target=efi-app-$(ARCH) $^ $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<
