        BITS 32
        SECTION .text
        GLOBAL io_hlt
        GLOBAL set_palette_rgb
        GLOBAL io_cli
        GLOBAL io_sti

io_hlt:
        HLT
        RET

io_cli:
        CLI
        RET

io_sti:
        STI
        RET

set_palette_rgb:
        push    ebp
        mov     ebp, esp
        mov     eax, [ebp + 8]
        mov     dx, 0x3c8
        out     dx, al
        mov     eax, [ebp + 12]
        mov     dx, 0x3c9
        out     dx, al
        mov     eax, [ebp + 16]
        out     dx, al
        mov     eax, [ebp + 20]
        out     dx, al
        pop     ebp
        ret

; void palette_init_with_cli(void) — PUSHFD / CLI / 写调色板 / POPFD
        GLOBAL palette_init_with_cli

palette_init_with_cli:
        pushfd
        push    eax
        push    edx
        cli

        mov     eax, 6                  ; index 6 → green
        mov     dx, 0x3c8
        out     dx, al
        mov     al, 0
        mov     dx, 0x3c9
        out     dx, al
        out     dx, al
        mov     al, 63
        out     dx, al

        pop     edx
        pop     eax
        popfd
        ret
