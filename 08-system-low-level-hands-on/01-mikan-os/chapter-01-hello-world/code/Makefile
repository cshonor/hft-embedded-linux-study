# Ch1 · 最小 UEFI Hello → BOOTX64.EFI
#
# 路径 A（默认）：clang -target x86_64-pc-win32-coff + lld-link  — 对齐 mikanos-build day01/c
# 路径 B（纯 LLVM）：clang --target=x86_64-elf -ffreestanding + ld.lld -flavor link  — make LINK=ld.lld
#
#   make                  → esp/EFI/BOOT/BOOTX64.EFI
#   make LINK=ld.lld      → 全程 ld.lld，不依赖 GCC 交叉链
#   make run              → QEMU + OVMF（WSL / Linux）
#   make CLANG=clang-18

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
# 纯 LLVM：freestanding + ld.lld PE 链接（Linux/WSL 常见，只需 llvm + lld 包）
CLANG_TARGET = x86_64-elf
CLANG_EXTRA  = -ffreestanding

hello.o: hello.c
	$(CLANG) --target=$(CLANG_TARGET) $(CLANG_EXTRA) -fshort-wchar -c -o $@ $<

$(EFI): hello.o
	mkdir -p $(ESP)/EFI/BOOT
	$(LD_LLD) -flavor link -subsystem:efi_application -entry:EfiMain $< -o $@
else
# 官方 day01/c 路径
CLANG_TARGET = x86_64-pc-win32-coff

hello.o: hello.c
	$(CLANG) -target $(CLANG_TARGET) -o $@ -c $<

$(EFI): hello.o
	mkdir -p $(ESP)/EFI/BOOT
	$(LINK) /subsystem:efi_application /entry:EfiMain /out:$@ $<
endif

run: $(EFI)
	$(QEMU) -bios $(OVMF) -drive format=raw,file=fat:rw:$(ESP) -m 512M

clean:
	rm -f hello.o
	rm -rf $(ESP)
