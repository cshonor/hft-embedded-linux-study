# Ch1 · 最小 UEFI Hello → BOOTX64.EFI
#
# 默认工具链：Clang + lld-link（对齐 mikanos-build day01/c）
# 全书 MikanOS 工程：mikanos-build 的 x86_64-elf-gcc + source buildenv.sh（Ch2+）
#
#   make              → esp/EFI/BOOT/BOOTX64.EFI
#   make run          → QEMU + OVMF
#   make CLANG=clang-18   # 覆盖编译器路径
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
