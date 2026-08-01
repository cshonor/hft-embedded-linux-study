; Day 3 · asmfunc — 32-bit stubs that C cannot express (HLT, IN/OUT, …)
; nasm -f elf32 asmfunc.asm -o asmfunc.o

        BITS 32
        SECTION .text
        GLOBAL io_hlt

io_hlt:
        HLT
        RET
