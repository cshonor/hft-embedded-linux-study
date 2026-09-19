/* keep_test.lds —— 测试 KEEP() 效果 */
SECTIONS
{
    . = 0x400000;
    .text : { *(.text) *(.text.*) }
    .rodata : { *(.rodata) *(.rodata.*) }

    /* 用 KEEP 的段 */
    .initcall_keep : {
        __initcall_keep_start = .;
        KEEP(*(.initcall_keep))
        __initcall_keep_end = .;
    }

    .data : { *(.data) *(.data.*) }
    .bss : { *(.bss) *(.bss.*) *(COMMON) }
    /DISCARD/ : { *(.comment) *(.note.*) }
}
