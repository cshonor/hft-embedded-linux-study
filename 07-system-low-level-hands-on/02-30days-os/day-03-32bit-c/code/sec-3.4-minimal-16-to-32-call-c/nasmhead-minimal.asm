; Teaching example · 16→32 switch + call C (not full haribote; see §3.4.3)
; Book equivalent: nasmhead.asm (minimal — no VGA / A20 / io_hlt)

        BITS 16

        GLOBAL switch_to_32

switch_to_32:
        cli
        lgdt [gdt_desc]

        mov     eax, cr0
        or      eax, 1
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

        extern  kernel_main
        call    kernel_main

hang:
        hlt
        jmp     hang

        BITS 16
align 8
gdt:
        dq      0
        dq      0x00cf9a000000ffff
        dq      0x00cf92000000ffff
gdt_end:

gdt_desc:
        dw      gdt_end - gdt - 1
        dd      gdt

CODE_SEL  equ 0x08
DATA_SEL  equ 0x10
