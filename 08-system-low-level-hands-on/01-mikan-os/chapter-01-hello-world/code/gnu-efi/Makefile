# GNU-EFI · Linux/WSL only
EFI_INC  ?= /usr/include/x86_64-linux-gnu/efi
EFI_LIB  ?= /usr/lib/x86_64-linux-gnu/gnuefi
CRT0     ?= $(EFI_LIB)/crt0-efi-x86_64.o
CC       ?= x86_64-linux-gnu-gcc

CFLAGS  = -ffreestanding -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I$(EFI_INC)
LDFLAGS = -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -L$(EFI_LIB)
LIBS    = -lefi -lgnuefi

OVMF    ?= /usr/share/OVMF/OVMF_CODE.fd
ESP     = ESP

.PHONY: all esp run clean

all: BOOTX64.EFI

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

BOOTX64.EFI: main.o $(CRT0)
	$(CC) $(LDFLAGS) $(CRT0) main.o -o $@ $(LIBS)

esp: BOOTX64.EFI
	mkdir -p $(ESP)/EFI/BOOT
	cp BOOTX64.EFI $(ESP)/EFI/BOOT/

run: esp
	qemu-system-x86_64 -bios $(OVMF) -drive format=raw,file=fat:rw:$(ESP) -m 256M

clean:
	rm -f main.o BOOTX64.EFI
	rm -rf $(ESP)
