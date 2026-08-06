/* 极简内核链接脚本模板（教学用，WSL/Linux ld） */
ENTRY(_start)

SECTIONS
{
    . = 0x80000;

    .text : {
        _stext = .;
        *(.text .text.*)
        _etext = .;
    }

    .rodata : {
        _srodata = .;
        *(.rodata .rodata.*)
        _erodata = .;
    }

    .data : {
        _sdata = .;
        *(.data .data.*)
        _edata = .;
    }

    .bss : {
        _sbss = .;
        *(.bss .bss.* COMMON)
        _ebss = .;
    }
}
