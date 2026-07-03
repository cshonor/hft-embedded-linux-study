# 工程 01 · Clang 极简 BOOTX64.EFI（WSL / Linux）
#   make       → esp/EFI/BOOT/BOOTX64.EFI
#   make run   → QEMU + OVMF
#   make clean → 删除 hello.o 和 esp/

CLANG ?= clang
LD_LLD ?= ld.lld
QEMU  ?= qemu-system-x86_64
OVMF  ?= /usr/share/OVMF/OVMF_CODE.fd

CLANG_TARGET = x86_64-elf
CLANG_EXTRA  = -ffreestanding

ESP   = esp
EFI   = $(ESP)/EFI/BOOT/BOOTX64.EFI

.PHONY: all run clean

all: $(EFI)

hello.o: hello.c
	$(CLANG) --target=$(CLANG_TARGET) $(CLANG_EXTRA) -fshort-wchar -c -o $@ $<

$(EFI): hello.o
	mkdir -p $(ESP)/EFI/BOOT
	$(LD_LLD) -flavor link -subsystem:efi_application -entry:EfiMain $< -o $@

run: $(EFI)
	$(QEMU) -bios $(OVMF) -drive format=raw,file=fat:rw:$(ESP) -m 512M

clean:
	rm -f hello.o
	rm -rf $(ESP)
