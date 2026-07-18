.global add_asm
.type add_asm, %function
add_asm:
    add r0, r0, r1    @ 返回值在 r0 (AAPCS)
    bx  lr
