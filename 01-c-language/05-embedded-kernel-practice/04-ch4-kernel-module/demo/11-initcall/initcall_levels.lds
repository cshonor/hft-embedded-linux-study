/* initcall_levels.lds —— 8 个级别段，模拟 vmlinux.lds.h 的 INIT_CALLS 宏 */
SECTIONS
{
    . = 0x400000;

    .text : { *(.text) *(.text.*) }
    .rodata : { *(.rodata) *(.rodata.*) }

    .initcall : {
        __initcall0_start = .;  KEEP(*(.initcall0.init))
        __initcall1_start = .;  KEEP(*(.initcall1.init))
        __initcall2_start = .;  KEEP(*(.initcall2.init))
        __initcall3_start = .;  KEEP(*(.initcall3.init))
        __initcall4_start = .;  KEEP(*(.initcall4.init))
        __initcall5_start = .;  KEEP(*(.initcall5.init))
        __initcall6_start = .;  KEEP(*(.initcall6.init))
        __initcall7_start = .;  KEEP(*(.initcall7.init))
        __initcall_end = .;
    }

    .data : { *(.data) *(.data.*) }
    .bss : { *(.bss) *(.bss.*) *(COMMON) }
    /DISCARD/ : { *(.comment) *(.note.*) }
}
