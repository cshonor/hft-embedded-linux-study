# Ch1 · 最小 UEFI Hello → BOOTX64.EFI
# 用法（WSL / Linux，需 clang + lld-link + qemu-system-x86_64 + OVMF）：
#   make          → esp/EFI/BOOT/BOOTX64.EFI
#   make run      → QEMU + OVMF 启动
#   make clean

CLANG ?= clang
LLD   ?= lld-link
QEMU  ?= qemu-system-x86_64

# Ubuntu: /usr/share/OVMF/OVMF_CODE.fd  ·  也可 export OVMF=/path/to/OVMF_CODE.fd
OVMF  ?= /usr/share/OVMF/OVMF_CODE.fd

ESP   = esp
EFI   = $(ESP)/EFI/BOOT/BOOTX64.EFI

.PHONY: all run clean

all: $(EFI)

hello.o: hello.c
	$(CLANG) -target x86_64-pc-win32-coff -o $@ -c $<

$(EFI): hello.o
	mkdir -p $(ESP)/EFI/BOOT
	$(LLD) /subsystem:efi_application /entry:EfiMain /out:$@ $<

run: $(EFI)
	$(QEMU) -bios $(OVMF) -drive format=raw,file=fat:rw:$(ESP) -m 512M

clean:
	rm -f hello.o
	rm -rf $(ESP)
