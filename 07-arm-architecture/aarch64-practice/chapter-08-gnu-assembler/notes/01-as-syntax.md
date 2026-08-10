# 8.1 GNU as 基本语法

> 来源：§8.1 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

GNU as 汇编器的基本语法：指令大小写、注释、标号、指令格式、立即数前缀。这是编写 AArch64 汇编程序的基础。

## 核心要点

### 语法风格对比

| 特性 | GNU as 风格 | ARM 官方手册风格 | 说明 |
|------|------------|-----------------|------|
| 指令大小写 | **小写**（推荐） | 大写 | GNU as 大小写都支持 |
| 注释 | `//` 或 `/* */` 或 `;` | `;` | 多种注释方式 |
| 标号 | `label:` 顶格 | 同 | 标号必须行首无空格 |
| 立即数 | `#imm` | `#imm` | 统一用 # 前缀 |
| 寄存器 | 直接写 `x0` | 直接写 `X0` | 无 $ 前缀（不同于 x86 AT&T） |

### 基本程序结构

```asm
.section .text           ; 代码段
.global _start           ; 声明全局入口符号

_start:                   ; 标号顶格
    mov x0, #42           ; 指令缩进
    mov x1, x0            ; 寄存器间传送
    add x2, x0, x1        ; x2 = x0 + x1
    // 程序结束
hang:
    wfe                   ; 等待事件（低功耗）
    b hang
```

### 标号规则

```asm
; 标号必须顶格（行首无空格）
label1:                   ; ✓ 顶格
    mov x0, #1

; 缩进的"标号"会被当作指令 → 报错
    label2:               ; ✗ 不是标号，被当作指令
    mov x0, #2

; 数字标号（局部标号）
1:
    mov x0, #1
    b 1b                  ; b = backward，跳回上面的标号 1
2:
    mov x1, #2
    b 2f                  ; f = forward，跳到下面的标号 2
```

### 注释方式

```asm
    mov x0, #1            ; 行注释（分号或 //）
    mov x1, #2            // 行注释（C++ 风格）
/* 块注释
   可以跨行 */
    mov x2, #3
```

### 立即数格式

```asm
    mov x0, #0xFF         ; 十六进制
    mov x0, #255          ; 十进制
    mov x0, #0b11111111   ; 二进制
    mov x0, #'A'          ; 字符字面量（0x41）
```

### 常用伪操作概览

| 伪操作 | 作用 |
|--------|------|
| `.section .text` | 切换到代码段 |
| `.global sym` | 声明全局符号 |
| `.align n` | 对齐 2^n 字节 |
| `.word/.quad` | 写 4/8 字节常量 |
| `.ascii/.asciz` | 写字符串 |
| `.equ name, val` | 定义常量 |
| `.macro/.endm` | 宏定义 |
| `.if/.endif` | 条件汇编 |

## 与 C 的对照

```c
// C 代码
int _start() {
    int a = 42;
    int b = a;
    int c = a + b;
    while(1) { __asm__("wfe"); }
}
```

```asm
// 等价的 GNU as 汇编
.section .text
.global _start
_start:
    mov x0, #42
    mov x1, x0
    add x2, x0, x1
hang:
    wfe
    b hang
```

## 常见错误

1. **标号不顶格**：缩进的标号被当作指令 → 汇编报错。
2. **.global 在标号之后**：`.global` 必须在标号定义前声明。
3. **混淆 ARM 官方大写和 GNU as 小写**：两者都支持，但混用可读性差。

## HFT 关联

汇编语法规范影响代码可读性和维护：
- 内核中 ARM64 汇编统一用 GNU as 小写风格
- 内联汇编也遵循 GNU as 语法
- HFT 中手动优化的热路径函数可能用汇编编写 → 统一风格便于团队维护
- 理解 `.directive` 伪操作对阅读内核汇编（如 head.S）至关重要

## 自测题

1. 以下代码有什么语法错误？
```asm
_start:
.global _start
    mov x0, #1
```
<details><summary>答案</summary>
`.global _start` 必须在标号 `_start:` 之前。伪操作 `.global` 声明全局符号，需在使用标号之前声明。
</details>

2. GNU as 中指令用大写还是小写？
<details><summary>答案</summary>
GNU as 大小写都支持，但推荐小写。ARM 官方手册用大写，但 GNU as 社区惯例是小写。本仓库统一用小写。
</details>

3. 标号和指令的缩进有什么规则？
<details><summary>答案</summary>
标号必须顶格（行首无空格），否则被当作指令。指令建议缩进以提高可读性。这是 GNU as 的语法要求。
</details>

4. `b 1b` 和 `b 1f` 分别跳到哪里？
<details><summary>答案</summary>
`1b`（backward）跳转到当前位置**之前**最近的数字标号 `1:`。`1f`（forward）跳转到当前位置**之后**最近的数字标号 `1:`。数字标号可以重复定义，`b`/`f` 后缀消除歧义。这是 GNU as 的局部标号机制，常用于循环和短跳转。
</details>

5. 以下立即数写法哪些合法？
```asm
mov x0, #0xFF
mov x0, #255
mov x0, #0b1010
mov x0, #'A'
```
<details><summary>答案</summary>
全部合法：
- `#0xFF` — 十六进制（255）
- `#255` — 十进制
- `#0b1010` — 二进制（10）
- `#'A'` — 字符字面量（0x41）

但注意：合法的立即数格式不等于 MOV 能编码。`#0xFF` 合法（16位内），但 `#0x12345678` 格式合法但 MOV 无法编码（需 MOVZ+MOVK）。
</details>

## 参考与延伸

- 原书 §8.1
- [8.2 伪指令](02-directives.md)
- [8.3 段](03-sections.md)
- GNU as Manual §3
