; Day 2 · 启动区 IPL（512 B）
; nasm -f bin helloos.asm -o ipl.bin
; 与 Day 1 引导扇区机器码一致 — 见 day-01 §1.3 对照表

        ORG     0x7C00

start:
        MOV     AX, 0
        MOV     SS, AX
        MOV     SP, 0x7C00
        MOV     DS, AX
        MOV     ES, AX
        MOV     SI, msg

putloop:
        MOV     AL, [SI]
        ADD     SI, 1
        CMP     AL, 0
        JE      fin
        MOV     AH, 0x0E
        MOV     BX, 0x000F
        INT     0x10
        JMP     putloop

fin:
        HLT
        JMP     fin

msg:
        DB      "hello, world", 0

        TIMES   510-($-$$) DB 0
        DW      0xAA55
