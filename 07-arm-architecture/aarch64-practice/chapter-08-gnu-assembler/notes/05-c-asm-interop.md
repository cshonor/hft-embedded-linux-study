# 8.5 C ↔ 汇编互调

> 来源：§8.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

C 函数调用汇编函数，以及汇编调用 C 函数——AAPCS64 调用约定的实践。这是混合编程的基础。

## 核心要点

### AAPCS64 调用约定

| 寄存器 | 用途 | 保存责任 | 说明 |
|--------|------|----------|------|
| X0-X7 | 参数 1-8 / 返回值 | caller-saved | 最多 8 个参数在寄存器中 |
| X8 | 间接返回地址 / syscall号 | caller-saved | 结构体返回或 Linux syscall |
| X9-X15 | 临时寄存器 | caller-saved | 调用者负责保存 |
| X16-X17 | IP0/IP1 | caller-saved | 链接器/PLT 用，可能被覆盖 |
| X18 | 平台寄存器 | — | Linux 保留，不要用 |
| X19-X28 | 被调用者保存 | callee-saved | 使用前必须保存 |
| X29 | FP 帧指针 | callee-saved | 调试回溯用 |
| X30 | LR 返回地址 | callee-saved | BL 自动设置 |
| SP | 栈指针 | — | 必须 16 字节对齐 |

### C 调用汇编函数

```c
// main.c
extern int my_add(int a, int b);  // 声明汇编函数

int main() {
    int result = my_add(3, 4);    // a=3→X0, b=4→X1
    return result;                 // 返回值在 X0
}
```

```asm
// func.s
.global my_add              ; 必须声明 .global，否则链接器找不到
my_add:
    add x0, x0, x1           ; X0 = X0 + X1（参数 a + b）
    ret                       ; 返回，X0 是返回值
```

**编译和链接**：
```bash
aarch64-linux-gnu-gcc -c main.c -o main.o
aarch64-linux-gnu-as func.s -o func.o
aarch64-linux-gnu-gcc main.o func.o -o test
```

### 汇编调用 C 函数

```asm
// start.s
.section .text
.global _start
_start:
    mov x0, #3               ; 参数 a = 3
    mov x1, #4               ; 参数 b = 4
    bl  c_add                 ; 调用 C 函数，返回值在 X0
    ; X0 = 7

    ; 退出程序
    mov x8, #93               ; exit syscall
    mov x0, #0
    svc #0
```

```c
// c_func.c
int c_add(int a, int b) {
    return a + b;             // a→X0, b→X1, 返回→X0
}
```

### 寄存器保存规则

```
caller-saved（X0-X15）：
  调用前如果需要这些寄存器的值，调用者自己保存
  被调函数可以自由修改，不需要恢复

callee-saved（X19-X28, X29, X30）：
  被调函数如果修改了这些寄存器，必须在返回前恢复原值
  调用者不需要保存，期望函数返回后值不变
```

```asm
; 正确：使用 X19 前保存，返回前恢复
my_func:
    stp x29, x30, [sp, #-16]!   ; 保存 FP 和 LR（X30 是 callee-saved）
    str x19, [sp, #-8]!          ; 保存 X19（callee-saved）
    ; ... 使用 X19 ...
    mov x19, x0                  ; 修改 X19
    ; ... 其他操作 ...
    ldr x19, [sp], #8            ; 恢复 X19
    ldp x29, x30, [sp], #16     ; 恢复 FP 和 LR
    ret

; X0-X7 可以自由使用，不需要保存
simple_func:
    add x0, x0, x1              ; 修改 X0（caller-saved）→ OK
    ret                          ; 不需要恢复 X0
```

### 超过 8 个参数

```asm
; C 函数: int func(int a1, int a2, ..., int a10)
; 参数 1-8 在 X0-X7
; 参数 9-10 在栈上

; 调用者：
    mov x0, #1                   ; 参数 1
    mov x1, #2                   ; 参数 2
    ; ... 参数 3-8 在 X2-X7 ...
    mov x8, #8                   ; 参数 8
    ; 参数 9 和 10 通过栈传递
    stp x9, x10, [sp, #-16]!    ; 参数 9=X9?, 参数 10=X10? → 不对
    ; 正确做法：
    mov x9, #9
    mov x10, #10
    stp x9, x10, [sp, #-16]!    ; 压栈（注意 16 字节对齐）
    bl  ten_param_func
    add sp, sp, #16              ; 清理栈（调用者负责）
```

### 结构体返回

```c
struct Point { int x, y; };

// 小结构体（≤16字节）通过 X0-X1 返回
struct Point make_point(int x, int y) {
    struct Point p = {x, y};
    return p;  // x→X0, y→X1
}

// 大结构体（>16字节）通过 X8 间接返回
struct Big { int data[100]; };
struct Big make_big() {
    struct Big b = {0};
    return b;
    // 调用者在 X8 中传入目标地址，函数把数据写入 [X8]
}
```

## 与 C 的对照

```c
// C 函数的寄存器使用规则由编译器自动处理
// 但在内联汇编中需要手动遵守

int add(int a, int b) {
    int result;
    asm volatile(
        "add %w[result], %w[a], %w[b]"
        : [result] "=r" (result)   // 输出
        : [a] "r" (a), [b] "r" (b) // 输入
    );
    return result;
}
```

## 常见错误

1. **忘记 .global**：汇编函数不加 `.global` → 链接器找不到符号。
2. **callee-saved 不保存**：用 X19 不保存 → 调用者数据被破坏。
3. **参数超过 8 个不正确处理**：第 9+ 参数在栈上，调用者需要压栈和清理。
4. **返回值类型搞错**：64 位返回值用 X0，32 位用 W0，不要搞混。

## HFT 关联

- 热路径函数用汇编手写 → 通过 AAPCS64 与 C 代码无缝衔接
- 参数最多 8 个在寄存器中传递 → 避免栈传参开销
- callee-saved 寄存器（X19-X28）需要保存恢复
- HFT 中尽量让函数参数 ≤ 8 → 全部寄存器传参，无栈开销

```asm
; HFT：汇编手写的热路径函数
; 参数全部在寄存器中，无栈操作
.global match_order
; X0 = order_ptr, X1 = best_price, X2 = qty
match_order:
    ldr x3, [x0]               ; 加载订单价格
    cmp x3, x1                  ; 比较
    csel x0, x3, x1, ge         ; 返回更优价格
    ret                          ; 仅用 caller-saved，无栈帧
```

## 自测题

1. C 函数 `int add(int a, int b)` 调用时，a 和 b 分别在哪个寄存器？
<details><summary>答案</summary>
a 在 X0，b 在 X1。返回值放 X0。
</details>

2. 汇编函数中可以使用 X19 吗？有什么要求？
<details><summary>答案</summary>
可以用，但 X19 是 callee-saved。使用前必须保存原值（压栈），返回前恢复。否则调用者的 X19 被破坏。
</details>

3. 如果 C 函数有 10 个参数，后 2 个怎么传？
<details><summary>答案</summary>
第 9-10 个参数通过栈传递。调用者在栈上分配空间，把参数写入 [sp+0] 和 [sp+8]。比寄存器传参慢。
</details>

4. 以下汇编函数有什么问题？
```asm
.global bad_func
bad_func:
    mov x19, x0      ; 使用 X19 存储参数
    bl   helper       ; 调用 C 函数
    mov x0, x19       ; 用 X19 恢复参数
    ret
```
<details><summary>答案</summary>
X19 是 callee-saved 寄存器，修改前必须保存原值。这里直接覆盖了 X19，返回后调用者的 X19 被破坏。修复：
```asm
bad_func:
    stp x29, x30, [sp, #-16]!
    str x19, [sp, #-8]!       ; 保存 X19
    mov x19, x0
    bl   helper
    mov x0, x19
    ldr x19, [sp], #8         ; 恢复 X19
    ldp x29, x30, [sp], #16
    ret
```
</details>

5. C 函数返回一个 `struct Point { int x, int y; }`（8字节），汇编函数如何返回？
<details><summary>答案</summary>
小结构体（≤16字节）通过寄存器返回。`Point` 是 8 字节，通过 X0 返回（x 在低 32 位 W0，y 在高 32 位）。或者 X0=x, X1=y（两个 32 位值分别放在两个寄存器的低 32 位）。具体取决于编译器实现，但 AAPCS64 规定 ≤16 字节的结构体用 X0-X1 返回。
</details>

## 参考与延伸

- 原书 §8.5
- [Ch21 调用约定与栈帧](../../chapter-21-os-topics/notes/section-0-本章完整概述.md)
- [Ch10 GCC 内联汇编](../../chapter-10-gcc-inline-asm/notes/section-0-本章完整概述.md)
