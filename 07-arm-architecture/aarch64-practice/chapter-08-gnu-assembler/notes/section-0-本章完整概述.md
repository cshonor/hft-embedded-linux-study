# Ch8 完整总结 · GNU 汇编器

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

学完 A64 指令集后，用 **GNU as** 编写完整汇编程序：伪指令、段、宏、C↔汇编互调。读 U-Boot/内核 `.S` 文件的基础。

实验优先 **QEMU** `-cpu cortex-a76`。

---

## 8.1 GNU as 基本语法

| 特性 | GNU as 风格 | 说明 |
|------|-------------|------|
| 大小写 | **小写** | `mov x0, x1`（官方手册大写，本书用小写） |
| 注释 | `;` 或 `/* */` | 行内用 `;`，块用 C 风格 |
| 立即数 | `#` 前缀 | `mov x0, #42` |
| 标签 | `label:` | 后面跟指令 |
| 寄存器 | `$` 无前缀 | 直接写 `x0`（不像 x86 AT&T 语法） |

```asm
.section .text
.global _start

_start:
    mov x0, #42        ; 立即数
    mov x1, x0         ; 寄存器
    add x2, x0, x1     ; x2 = x0 + x1
```

---

## 8.2 常用伪指令/伪操作 ⭐

伪指令不是真正的机器指令，是汇编器提供的便利写法，编译时会展开成真指令。

| 伪指令 | 作用 | 展开为 |
|--------|------|--------|
| `.global symbol` | 导出全局符号 | — |
| `.section .text` | 切换到 text 段 | — |
| `.data` | 切换到 data 段 | — |
| `.bss` | 切换到 bss 段 | — |
| `.align n` | 对齐到 2^n 字节 | — |
| `.word val` | 写 4 字节 | — |
| `.quad val` | 写 8 字节 | — |
| `.asciz "str"` | 写 C 字符串（带 \0） | — |
| `.byte val` | 写 1 字节 | — |
| `.space n` | 填充 n 字节零 | — |
| `ldr x0, =val` | 加载大常量/地址 | litpool + 真 LDR |
| `.equ name, val` | 定义常量 | — |
| `.macro / .endm` | 宏定义 | — |

---

## 8.3 段（Section）⭐

```asm
.section .text          ; 代码段（只读、可执行）
.section .data          ; 数据段（读写、已初始化）
.section .bss           ; BSS 段（读写、未初始化，全零）
```

| 段 | 特性 | 说明 |
|----|------|------|
| `.text` | RX | 代码 |
| `.data` | RW | 已初始化全局变量 |
| `.bss` | RW | 未初始化/初始化为 0 的变量（不占文件空间） |
| `.rodata` | R | 只读数据（字符串常量） |

> **链接器**（Ch9）会把所有 `.text` 段合并、所有 `.data` 段合并，按链接脚本排列在内存中。

---

## 8.4 宏

```asm
.macro putc char
    ldr x0, =\char
    bl  putchar
.endm

    putc "H"
    putc "i"
```

- `\char` 是宏参数引用（反斜线前缀）
- 宏在汇编阶段展开，不产生运行时开销

---

## 8.5 C ↔ 汇编互调 ⭐

### C 调汇编函数

```c
// main.c
extern int my_add(int a, int b);
int main() { return my_add(3, 4); }
```

```asm
// func.s
.global my_add
my_add:
    add x0, x0, x1     ; x0=a, x1=b (AAPCS64 参数传递)
    ret
```

### 汇编调 C 函数

```asm
    // 设置参数后 BL 调用 C 函数
    mov x0, #3
    mov x1, #4
    bl  c_add           ; 返回值在 x0
```

```c
int c_add(int a, int b) { return a + b; }
```

**AAPCS64 调用约定（AArch64 Procedure Call Standard）：**

| 寄存器 | 用途 |
|--------|------|
| X0-X7 | 参数 / 返回值 |
| X8 | 间接返回地址（结构体返回）/ syscall number |
| X9-X15 | 临时寄存器（caller-saved） |
| X16-X17 | IP0/IP1（链接器/PLT 用） |
| X18 | 平台寄存器 |
| X19-X28 | callee-saved（被调方负责保存） |
| X29 | FP 帧指针 |
| X30 | LR 返回地址 |
| SP | 栈指针（16 字节对齐） |

> **caller-saved**：调用者保存（调用前如果需要，自己压栈）  
> **callee-saved**：被调者保存（函数入口自己保存，出口恢复）

---

## 8.6 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 8-1 | 汇编求最大数 | QEMU |
| 8-2 | C 调汇编函数 | QEMU |
| 8-3 | 汇编调 C 函数 | QEMU |
| 8-4 | 用伪操作实现表 | QEMU |
| 8-5 | 汇编宏的使用 | QEMU |

---

## 8.7 易错点清单

1. **C 调汇编函数没 `.global`** → 链接器找不到符号。
2. **参数超过 8 个** → X0-X7 不够，第 9 个参数通过栈传递。
3. **callee-saved 寄存器没保存** → X19-X28 被调用方负责，用了必须保存恢复。
4. **SP 不对齐** → AAPCS64 要求 16 字节对齐。
5. **`.bss` 段写初始值** → BSS 应全零，写非零值会被链接器忽略或报错。

---

## 书中思考题（自测）

1. `.data` 和 `.bss` 段的区别？BSS 为什么不占文件空间？
2. AAPCS64 中哪些寄存器是 caller-saved？哪些是 callee-saved？
3. C 函数 `int foo(int a, int b, ..., int j)` 有 10 个参数，第 9 和第 10 个怎么传？
4. `.global` 的作用是什么？不加会怎样？
5. 宏在什么时候展开？有运行时开销吗？

**参考答案：**

1. `.data` 存**已初始化**变量（占文件空间）；`.bss` 存**未初始化/零**变量，启动时清零即可，不占文件空间。  
2. Caller-saved: **X0-X7, X9-X15**；Callee-saved: **X19-X28**。  
3. 第 9、10 个参数通过**栈**传递。  
4. 导出全局符号，使链接器可见；不加则**局部符号**，其他文件无法链接。  
5. 汇编阶段展开；**无运行时开销**。

---

上一章 [Ch7 工程陷阱](../../chapter-07-a64-traps/) · 下一章 [Ch9 链接器](../../chapter-09-linker-scripts/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
