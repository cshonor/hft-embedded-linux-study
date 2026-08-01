; Day 3 · nasmhead — bootpack entry loaded at 0x8200 by IPL
; 16-bit: VGA (§3.2), A20, GDT, CR0 → 32-bit → call HariMain
; nasm -f elf32 nasmhead.asm -o nasmhead.o

        BITS 16
        SECTION .text
        GLOBAL start

start:
        cli

        ; §3.2 — switch to VGA mode 0x13 (320×200×256); must run in 16-bit (BIOS)
        mov     ax, 0x0013
        int     0x10

        ; Enable A20 (fast gate on PS/2 class hardware; book uses keyboard controller)
        in      al, 0x92
        or      al, 2
        out     0x92, al

        lgdt    [gdt_desc]

        mov     eax, cr0
        or      eax, 1              ; CR0.PE = 1
        mov     cr0, eax

        jmp     CODE_SEL:init_pm

        BITS 32
init_pm:
        mov     ax, DATA_SEL
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax
        mov     esp, 0x90000

        extern  HariMain
        call    HariMain            ; hand off to C (bootpack.c)

hang:
        hlt
        jmp     hang

        BITS 16
align 8
gdt:
        dq      0
        dq      0x00cf9a000000ffff  ; code: flat 4G, execute/read
        dq      0x00cf92000000ffff  ; data: flat 4G, read/write
gdt_end:

gdt_desc:
        dw      gdt_end - gdt - 1
        dd      gdt

CODE_SEL  equ 0x08
DATA_SEL  equ 0x10
