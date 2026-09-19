/* initcall.lds —— 模拟内核链接脚本的 initcall 段收集
 * 对应 include/asm-generic/vmlinux.lds.h 的 INIT_CALLS 宏。
 * 核心：用 KEEP() + __start/__end 符号导出段边界。
 *
 * 用法: gcc ... -T initcall.lds -Wl,--gc-sections
 */
SECTIONS
{
    . = 0x400000;

    .text : {
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    /* 模拟 INIT_CALLS：收集所有 .initcall6.init 段 */
    .initcall : {
        __initcall_start = .;
        KEEP(*(.initcall6.init))
        __initcall_end = .;
    }

    .data : { *(.data) *(.data.*) }
    .bss : { *(.bss) *(.bss.*) *(COMMON) }

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}
