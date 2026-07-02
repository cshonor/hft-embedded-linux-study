## ④.3 16 切 32 与 call C 完整例子

**汇编搭台，C 唱戏。** 教学文件：[sec-3.4-minimal-16-to-32-call-c/](../code/sec-3.4-minimal-16-to-32-call-c/)（原书入口 **`HariMain`**，示例写 **`kernel_main`** 便于对照）。

---

### ① 汇编 — `nasmhead-minimal.asm`

```nasm
        BITS 16
switch_to_32:
        cli
        lgdt [gdt_desc]
        mov     eax, cr0
        or      eax, 1              ; CR0.PE = 1
        mov     cr0, eax
        jmp     CODE_SEL:init_pm

        BITS 32
init_pm:
        mov     ax, DATA_SEL
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     esp, 0x90000

        extern  kernel_main
        call    kernel_main         ; ★ 舞台搭好，请 C 上场

hang:
        hlt
        jmp     hang
```

GDT 完整定义见 [nasmhead-minimal.asm](../code/sec-3.4-minimal-16-to-32-call-c/nasmhead-minimal.asm)。

---

### ② C — `kernel_main.c`

```c
void kernel_main(void) {
    volatile char *vidmem = (volatile char *)0xB8000;
    const char *str = "Hello 32-bit C!";
    int i;

    for (i = 0; str[i] != '\0'; i++) {
        vidmem[i * 2]     = str[i];
        vidmem[i * 2 + 1] = 0x07;
    }
    for (;;) { }
}
```

切模式归 **asm**；`for` 循环归 **C**。

---

### ③ 链接与运行时

```text
nasmhead-minimal.asm  ──nasm──►  nasmhead.o
kernel_main.c         ──gcc -m32──►  kernel_main.o
                                        ld
                                        ▼
                                   bootpack.elf
```

**运行时：**

```text
switch_to_32（16 位 asm）
    → init_pm（32 位 asm：段、栈）
        → call kernel_main（C）
            → 写 "Hello 32-bit C!"
                → 若返回 → hang（asm HLT）
```

---

### ④ 与原书 haribote 对应

| 教学示例 | 原书 Day 3 |
|----------|------------|
| `switch_to_32` / GDT / CR0 | **nasmhead.asm** |
| `call kernel_main` | **`HariMain`** |
| `kernel_main` 写显存 | **bootpack.c** |
| `io_hlt` | **asmfunc.asm** |
| IPL 读盘 | **[ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm)** |
| 四文件 bootpack | **[sec-3.4-bootpack-asm-and-c/](../code/sec-3.4-bootpack-asm-and-c/)** |

链接命令：[sec-3.4-minimal-16-to-32-call-c/README.md](../code/sec-3.4-minimal-16-to-32-call-c/README.md) · 完整 bootpack：[sec-3.4-bootpack-asm-and-c/README.md](../code/sec-3.4-bootpack-asm-and-c/README.md)

---

← [§3.4.2 io_hlt](./section-3.4.2-io_hlt与工程分层.md) · [§3.4.4 嵌入式/HFT →](./section-3.4.4-嵌入式HFT与何时用汇编.md)
