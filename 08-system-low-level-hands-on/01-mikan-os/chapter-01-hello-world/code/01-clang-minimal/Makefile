# 工程 01 · Clang 极简 BOOTX64.EFI（WSL / Linux）
#   make       → esp/EFI/BOOT/BOOTX64.EFI
#   make run   → QEMU + OVMF
#   make clean → 删除 hello.o 和 esp/
#
# WSL 上与官方 day01/c 一致：COFF 对象 + lld-link 链 PE/EFI

CLANG ?= clang
LINK  ?= $(shell command -v lld-link 2>/dev/null || command -v lld-link-18 2>/dev/null || echo lld-link)
QEMU  ?= qemu-system-x86_64
OVMF  ?= /usr/share/OVMF/OVMF_CODE.fd

CLANG_TARGET = x86_64-pc-win32-coff

ESP   = esp
EFI   = $(ESP)/EFI/BOOT/BOOTX64.EFI

.PHONY: all run clean

all: $(EFI)

hello.o: hello.c
	$(CLANG) -target $(CLANG_TARGET) -ffreestanding -fshort-wchar -c -o $@ $<

$(EFI): hello.o
	mkdir -p $(ESP)/EFI/BOOT
	$(LINK) /subsystem:efi_application /entry:EfiMain $< /out:$@

run: $(EFI)
	$(QEMU) -bios $(OVMF) -drive format=raw,file=fat:rw:$(ESP) -m 512M

clean:
	rm -f hello.o
	rm -rf $(ESP)
