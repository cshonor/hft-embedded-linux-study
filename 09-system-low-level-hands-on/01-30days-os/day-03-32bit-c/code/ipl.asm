; Day 3 · 真正 IPL（读盘装载 bootpack）
; 原书 ipl.nas / haribote-ipl
; nasm -f bin ipl.asm -o ipl.bin

CYLS    EQU     10              ; 读取柱面数 C0~C9

        ORG     0x7C00

        JMP     entry
        DB      0x90
        DB      "HARIBOTE"
        DW      512             ; bytes per sector
        DB      1               ; sectors per cluster
        DW      1               ; reserved sectors
        DB      2               ; number of FATs
        DW      224             ; root directory entries
        DW      2880            ; total sectors (1.44MB)
        DB      0xF0            ; media descriptor
        DW      9               ; sectors per FAT
        DW      18              ; sectors per track
        DW      2               ; number of heads
        DD      0
        DD      2880
        DB      0, 0, 0x29
        DD      0xFFFFFFFF
        DB      "HARIBOTEOS "
        DB      "FAT12   "
        RESB    18

entry:
        MOV     AX, 0
        MOV     SS, AX
        MOV     SP, 0x7C00
        MOV     DS, AX

        MOV     AX, 0x0820
        MOV     ES, AX
        MOV     CH, 0             ; cylinder 0
        MOV     DH, 0             ; head 0
        MOV     CL, 2             ; sector 2 (S1 = this IPL)

readloop:
        MOV     SI, 0

retry:
        MOV     AH, 0x02
        MOV     AL, 1
        MOV     BX, 0
        MOV     DL, 0x00
        INT     0x13
        JNC     next
        ADD     SI, 1
        CMP     SI, 5
        JAE     error
        MOV     AH, 0x00
        MOV     DL, 0x00
        INT     0x13
        JMP     retry

next:
        MOV     AX, ES
        ADD     AX, 0x0020
        MOV     ES, AX
        ADD     CL, 1
        CMP     CL, 18
        JBE     readloop
        MOV     CL, 1
        ADD     DH, 1
        CMP     DH, 2
        JB      readloop
        MOV     DH, 0
        ADD     CH, 1
        CMP     CH, CYLS
        JB      readloop

        MOV     SI, msg_ok
putloop_ok:
        MOV     AL, [SI]
        ADD     SI, 1
        CMP     AL, 0
        JE      fin
        MOV     AH, 0x0E
        MOV     BX, 15
        INT     0x10
        JMP     putloop_ok

fin:
        JMP     0x8200

error:
        MOV     SI, msg_err
putloop_err:
        MOV     AL, [SI]
        ADD     SI, 1
        CMP     AL, 0
        JE      fin_err
        MOV     AH, 0x0E
        MOV     BX, 15
        INT     0x10
        JMP     putloop_err

fin_err:
        HLT
        JMP     fin_err

msg_ok:
        DB      0x0A, 0x0A
        DB      "load done", 0

msg_err:
        DB      0x0A, 0x0A
        DB      "load error", 0

        TIMES   510-($-$$) DB 0
        DW      0xAA55
