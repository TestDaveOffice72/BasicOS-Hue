ARCH          = x86_64

OUTDIR        = bin
HEADERS       = $(wildcard src/*.h)
OBJS          = $(patsubst src/%.c,$(OUTDIR)/%.o,$(wildcard src/*.c))
APPS          = $(patsubst src/%.s,$(OUTDIR)/%.com,$(wildcard src/*.s))
APPS_O        = $(patsubst src/%.s,$(OUTDIR)/app_%.o,$(wildcard src/*.s))

EFIINC        = /usr/include/efi
EFIINCS       = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
EFI_CRT_OBJS  = /usr/lib/crt0-efi-$(ARCH).o
EFI_LDS       = /usr/lib/elf_$(ARCH)_efi.lds
OVMF          = ovmf_x64.bin
QEMU_OPTS     = -enable-kvm -m 64 -serial file:debug.log -device VGA

CFLAGS        = $(EFIINCS) -xc -std=c11 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wno-incompatible-library-redeclaration
ifeq ($(ARCH),x86_64)
  CFLAGS += -DHAVE_USE_MS_ABI
endif
LDFLAGS       = -fuse-ld=gold -fpic -nostdlib -Wl,-znocombreloc,-T,$(EFI_LDS),-shared,-Bsymbolic,-L,/usr/lib
LD            = $(CC)

ifdef DEBUG
  CFLAGS += -g
else
  CFLAGS += -O3 -flto
  LDFLAGS += -flto
endif

all: $(OUTDIR)/huehuehuehuehue.efi $(OUTDIR)/huehuehuehuehue.sym

run: all
	qemu-system-$(ARCH) -pflash $(OVMF) -drive file=fat:$(OUTDIR),format=raw $(QEMU_OPTS)
	sed -n '/Kernel booting/,$$p' debug.log

$(OUTDIR)/huehuehuehuehue.so: $(OBJS) $(APPS_O) $(EFI_CRT_OBJS)
# TODO: remove -lefi in non-debug builds
	$(LD) $(LDFLAGS) -o $@ $^ -lgnuefi

$(OUTDIR)/%.efi: $(OUTDIR)/%.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .reloc --target=efi-app-$(ARCH) $^ $@
	strip -s $@

$(OUTDIR)/%.sym: $(OUTDIR)/%.so
	objcopy --only-keep-debug $< $@

$(APPS): $(OUTDIR)/%.com: src/%.s
	nasm -f bin $< -o $@

$(APPS_O): $(OUTDIR)/app_%.o: $(OUTDIR)/%.com
	objcopy -I binary -O elf64-$(subst _,-,$(ARCH)) -B i386 $< $@


$(OBJS): $(OUTDIR)/%.o: src/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# Utility rules for generating assembly and llvm-ir
ASMS = $(patsubst $(OUTDIR)/%.o,asm/%.s,$(OBJS))
LLIRS = $(patsubst $(OUTDIR)/%.o,asm/%.ll,$(OBJS))

asm: $(ASMS) $(LLIRS)

$(ASMS): asm/%.s: src/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(filter-out -flto,$(CFLAGS)) -S -o $@ $<
$(LLIRS): asm/%.ll: src/%.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(filter-out -flto,$(CFLAGS)) -S -emit-llvm -o $@ $<
