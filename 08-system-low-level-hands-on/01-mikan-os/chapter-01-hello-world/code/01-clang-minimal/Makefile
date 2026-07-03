# 工程 01 · Clang 极简 BOOTX64.EFI
#   make              → esp/EFI/BOOT/BOOTX64.EFI（默认 lld-link / win32-coff）
#   make LINK=ld.lld  → WSL/Linux 纯 ld.lld 路径
#   make run          → QEMU + OVMF

CLANG ?= clang
LINK  ?= lld-link
LD_LLD ?= ld.lld
QEMU  ?= qemu-system-x86_64
OVMF  ?= /usr/share/OVMF/OVMF_CODE.fd

ESP   = esp
EFI   = $(ESP)/EFI/BOOT/BOOTX64.EFI

.PHONY: all run clean

all: $(EFI)

ifeq ($(LINK),ld.lld)
CLANG_TARGET = x86_64-elf
CLANG_EXTRA  = -ffreestanding

hello.o: hello.c
	$(CLANG) --target=$(CLANG_TARGET) $(CLANG_EXTRA) -fshort-wchar -c -o $@ $<

$(EFI): hello.o
	mkdir -p $(ESP)/EFI/BOOT
	$(LD_LLD) -flavor link -subsystem:efi_application -entry:EfiMain $< -o $@
else
CLANG_TARGET = x86_64-pc-win32-coff

hello.o: hello.c
	$(CLANG) -target $(CLANG_TARGET) -ffreestanding -fshort-wchar -c -o $@ $<

$(EFI): hello.o
	mkdir -p $(ESP)/EFI/BOOT
	$(LINK) /subsystem:efi_application /entry:EfiMain /out:$@ $<
endif

run: $(EFI)
	$(QEMU) -bios $(OVMF) -drive format=raw,file=fat:rw:$(ESP) -m 512M

clean:
	rm -f hello.o
	rm -rf $(ESP)
