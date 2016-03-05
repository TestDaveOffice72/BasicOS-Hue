ARCH          = x86_64

OUTDIR        = bin
HEADERS       = src/kernel.h
OBJS          = $(patsubst src/%.c,$(OUTDIR)/%.o,$(wildcard src/*.c))

EFIINC        = /usr/include/efi
EFIINCS       = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
EFI_CRT_OBJS  = /usr/lib/crt0-efi-$(ARCH).o
EFI_LDS       = /usr/lib/elf_$(ARCH)_efi.lds
OVMF          = /usr/share/ovmf/ovmf_x64.bin
QEMU_OPTS     = -enable-kvm -m 64 -device VGA

CFLAGS        = $(EFIINCS) -xc -std=c11 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone \
-Wall -Wno-incompatible-library-redeclaration -O3 -flto
ifeq ($(ARCH),x86_64)
  CFLAGS += -DHAVE_USE_MS_ABI
endif
LDFLAGS       = -fuse-ld=gold -flto -nostdlib -Wl,-znocombreloc,-T,$(EFI_LDS),-shared,-Bsymbolic,-L,/usr/lib
LD            = $(CC)

all: $(OUTDIR)/huehuehuehuehue.efi


run: $(OUTDIR)/huehuehuehuehue.efi
	qemu-system-$(ARCH) -bios $(OVMF) -drive file=fat:$(OUTDIR),format=raw $(QEMU_OPTS)

$(OUTDIR)/huehuehuehuehue.so: $(OBJS) $(EFI_CRT_OBJS)
	$(LD) $(LDFLAGS) -o $@ $? -lgnuefi

$(OUTDIR)/%.efi: $(OUTDIR)/%.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .reloc --target=efi-app-$(ARCH) $^ $@
	strip -s $@

$(OBJS): $(OUTDIR)/%.o: src/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<
