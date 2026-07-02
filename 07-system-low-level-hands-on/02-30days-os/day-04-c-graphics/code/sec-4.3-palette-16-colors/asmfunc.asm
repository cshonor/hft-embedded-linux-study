        BITS 32
        SECTION .text
        GLOBAL io_hlt
        GLOBAL set_palette_rgb

io_hlt:
        HLT
        RET

; void set_palette_rgb(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
set_palette_rgb:
        push    ebp
        mov     ebp, esp
        mov     eax, [ebp + 8]          ; index
        mov     dx, 0x3c8
        out     dx, al
        mov     eax, [ebp + 12]         ; r
        mov     dx, 0x3c9
        out     dx, al
        mov     eax, [ebp + 16]         ; g
        out     dx, al
        mov     eax, [ebp + 20]         ; b
        out     dx, al
        pop     ebp
        ret
