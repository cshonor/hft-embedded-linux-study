# ABI · Application Binary Interface（应用二进制接口）

> **01 CSAPP · Ch2 §2.1.2 延伸阅读** · 解释 `sizeof` 为何随平台变、结构体布局、函数怎么调  
> **关联：** [§2.1.2 数据大小与 sizeof](./section-2.1.2-数据大小与sizeof.md) · [§2.1.3 字节序](./section-2.1.3-寻址与字节序.md) · [§3.7 调用约定](../../chapter-03-machine-level-programs/notes/section-3.7-过程与栈帧.md) · [§3.8 指针步长](../../chapter-03-machine-level-programs/notes/section-3.8-指针步长详解.md)

---

## 1. 全称

**Application Binary Interface** — **应用二进制接口**

---

## 2. 一句话核心 · 协议类比

> **ABI = 机器之间的二进制通信协议。**  
> 协议本质：双方提前约定固定规则，否则无法交互。

| | |
|--|--|
| **谁必须遵守** | 编译产物、CPU、OS 内核、动态库（`.so`/`.dll`） |
| **没协议时** | 传参、栈、系统调用各玩各的 → 直接崩 |
| **同一套 ABI** | 分开编译的 `.o` 能链接；别人编好的 `.so` 你能直接调 |

- **源代码** 是给人看的（API、语法、头文件）  
- **ABI** 是 **编译后的二进制** 与硬件/内核之间的 **强制沟通规则**

两套程序（`.o`、动态库、不同编译器产物）**不需要源码**，只要遵守 **同一套 ABI**，就能互相链接、调用、运行。

---

## 3. ABI 规定了什么（对应 CSAPP 章节）

| # | ABI 管什么 | 在 CSAPP 哪里 |
|---|------------|---------------|
| 1 | **基础类型占几字节** | Ch2 §2.1.2 — LP64 下 `long` 8 字节，LLP64（Win x64）下 `long` 4 字节 → **`sizeof` 不同的根源** |
| 2 | **结构体对齐、padding** | 例：`struct {char a; long b;}` 在 LP64 下常 **16 字节**（`a` 后补 7）→ Ch3 §3.9 · [ch02 demo](../../code/ch02-endian-and-padding-demo.c) |
| 3 | **函数调用约定** | Ch3 §3.7 — 传参/返回/caller·callee-save；**全景 → 下文 §6** |
| 4 | **栈对齐、结构体/位域布局** | §6.2–6.3 · Ch3 §3.9 |
| 5 | **ELF 段、动态链接 PLT/GOT** | Ch7 · §6.4 / §6.8 |
| 6 | **syscall、信号上下文、SIMD** | §6.5–6.7 · Ch8 |
| 7 | **指针宽度、符号命名、链接名修饰** | LP64；C++ mangling；`extern "C"` |
| 8 | **endian** | Ch2 §2.1.3 |

---

## 4. API vs ABI（源码协议 vs 二进制协议）

| | **API** | **ABI** |
|--|---------|---------|
| **本质** | 源码层「开发协议」 | **二进制层「通信协议」** |
| **给谁看** | 程序员 ↔ 编译器 | **elf/机器码 ↔ CPU ↔ OS ↔ 动态库** |
| **约定什么** | 函数名、参数**类型**、头文件声明 | 寄存器传参、栈布局、结构体内存排布、系统调用方式 |
| **约束何时** | 只约束源码；不同编译器编完后二进制规则可能不同 | **运行时真正生效**的底层协议 |
| **例子** | `int foo(long x);` 声明 | Linux x64：`foo` 首参进 `%rdi`；`long` 在 LP64 下占 8 字节 |

**同一套 C 源码：**

```text
Linux GCC/Clang  →  System V AMD64 ABI  →  .elf（可互调）
Windows MSVC     →  Microsoft x64 ABI   →  .exe
```

API 看起来一样，**ABI 不同 → 协议不兼容 → 二进制不能互换**（Linux 程序不能直接当 Windows 二进制跑）。

### 两个熟悉的 ABI 例子

**① ARM AAPCS**（ARM 架构过程调用标准 · → [Smith Ch13](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-13-subroutines-stacks/notes/section-13-5-apcs.md)）

| 条款（直觉） | |
|--------------|--|
| 前 4 个整型参数 | **R0–R3** |
| 栈对齐 | 常要求 **8 字节** |
| callee-save | **R4–R11** 子函数须保存/恢复 |

汇编不遵守 → 调 C 函数参数全错、程序飞掉。

**② x86-64 System V ABI**（Linux / HFT 服务器 · → [Ch3 §3.7](../../chapter-03-machine-level-programs/notes/section-3.7-过程与栈帧.md)）

整型参数依次：**`%rdi, %rsi, %rdx, %rcx, %r8, %r9`**（再溢出到栈）。

### 协议的通用性

| 场景 | 结果 |
|------|------|
| 同一架构 + 同一 OS + 同一 ABI | gcc/clang 编的库常可互调（Linux x86_64 → System V） |
| 换 OS / 换 Windows x64 ABI | **协议不兼容**，不能直接跑对方二进制 |

---

## 5. 典型 ABI 速查（64 位）

| 类型 | ILP32（32 位） | LP64（Linux/macOS x64） | LLP64（Windows x64） |
|------|----------------|-------------------------|----------------------|
| `int` | 4 | 4 | 4 |
| `long` | 4 | **8** | **4** |
| 指针 | 4 | 8 | 8 |

→ 本机验证：[ch02-endian-and-padding-demo.c](../../code/ch02-endian-and-padding-demo.c) 的 `demo_sizeof()`

---

## 6. x86-64 System V ABI 全景（Linux · 除「类型占几字节」以外）

> **数据类型长度只是 ABI 的地基一角。**  
> ABI 统一 **传参/返回、寄存器谁保存、栈怎么齐、结构体怎么排、ELF 各段、syscall、信号、SIMD、动态链接** — 让编译器、汇编、链接器、内核、`.so` 能互操作。  
> 细节展开：调用 → [Ch3 §3.7](../../chapter-03-machine-level-programs/notes/section-3.7-过程与栈帧.md) · 结构体 → [§3.9](../../chapter-03-machine-level-programs/notes/section-3.9-结构体联合与对齐.md) · 链接 → [Ch7](../../chapter-07-linking/) · 浮点 → [§3.11](../../chapter-03-machine-level-programs/notes/section-3.11-浮点代码.md)

### 0 · 你已会：类型与简单对齐（地基）

| C 类型（LP64） | 字节 | 典型对齐 |
|----------------|------|----------|
| `char` | 1 | 1 |
| `short` | 2 | 2 |
| `int` | 4 | 4 |
| `long` / 指针 / `long long` | 8 | 8 |

成员按自身对齐；结构体整体对齐 = 最大成员对齐 → **padding**。

**贴身例子（ABI 第 2 项）：**

```c
struct Test {
    char a;   /* offset 0，1 字节 */
    long b;   /* LP64：须 8 字节对齐 → a 后自动补 7 空字节；b 在 offset 8 */
};
/* Linux LP64：sizeof(struct Test) == 16 */
```

切到另一套对齐规则（如 ILP32 下 `long` 只按 4 对齐，或 LLP64 下 `long` 本身只有 4 字节），**补的字节数、成员偏移、总大小全变**。跨 ABI 把结构体当地址/二进制互传，对方按自己的 offset 读 → **直接读错或崩** — 这正是 ABI 管结构体对齐、也是跨 ABI 调用易炸的核心原因之一。

### 1 · 函数调用约定（CSAPP 必考）

**整型 / 指针参数：**

| 第 1–6 个 | 第 7 个起 |
|-----------|-----------|
| `%rdi, %rsi, %rdx, %rcx, %r8, %r9` | **栈**（布局上从右往左压参的习惯仍影响栈上顺序） |

**浮点参数：** `%xmm0`–`%xmm7`（与 GPR **分开计数**）。

**结构体传参（直觉）：** 小块常拆进寄存器；过大则常 **传指针 / 在栈上构造**（具体以 ABI 分类规则为准；手写 asm 以编译器 `-S` 为准）。

**返回值：**

| 类型 | 放哪 |
|------|------|
| 整数 / 指针 | `%rax`（128 位整数时常 **`%rdx:%rax`**） |
| 浮点 | `%xmm0` |

**寄存器归属：**

| 类 | 寄存器 | 谁负责 |
|----|--------|--------|
| **caller-saved** | `%rax,%rdi,%rsi,%rdx,%rcx,%r8–%r11` | 调用前若还要用 → **自己先存**；被调方可随意改 |
| **callee-saved** | `%rbx,%rbp,%r12–%r15` | 被调方若用 → **进函数保存、返回前恢复** |

### 2 · 栈布局与对齐

1. 栈向 **低地址** 增长；`%rsp` = 栈顶。  
2. **`call` 前 `%rsp` 须 16 字节对齐**（兼容 SSE；不齐 → 浮点/向量路径易炸）。  
3. 常用帧：`%rbp` 基址；局部多在 **负偏移**，旧帧/返回址/部分参数在 **正偏移**（如 `8(%rbp)`）。  
4. 栈对象 **随函数返回失效** — 勿返回局部地址跨函数用。

### 3 · 结构体 / 联合 / 位域

- 整体对齐 = 最大基础成员对齐；自动 **padding**。  
- **union** 按最大成员占坑。  
- **bit-field** 分配顺序与跨单元规则由 ABI 写死（跨编译器勿假设可移植裸布局）。  
→ [§3.9](../../chapter-03-machine-level-programs/notes/section-3.9-结构体联合与对齐.md)

### 4 · ELF 分段（全局 / 静态 / 常量落哪）

| 段 | 放什么 |
|----|--------|
| **`.text`** | 机器码（函数） |
| **`.rodata`** | 只读常量、字符串字面量 |
| **`.data`** | 已初始化全局/静态 |
| **`.bss`** | 未初始化全局/静态（加载时清零） |

另含符号、重定位、动态节 — → [Ch7 链接](../../chapter-07-linking/)

### 5 · 系统调用（用户态 → 内核）

| | Linux **x86-64** | Linux **AArch64**（对照） |
|--|------------------|---------------------------|
| 触发 | `syscall` | `svc #0` |
| 调用号 | **`%rax`** | **`x8`** |
| 参数 | `%rdi,%rsi,%rdx,%r10,%r8,%r9`（注意第 4 个是 **`r10`** 不是 `rcx`） | `x0`–`x5` |
| 返回 | `%rax`；负值常表示 errno 风格错误 | `x0` |

### 6 · 信号 / 异常上下文

收到信号或陷入时：内核如何 **保存寄存器快照**、是否切 **信号栈**、handler 的调用约定 — 亦属 ABI/内核约定交叉带。→ [Ch8 ECF](../../chapter-08-exceptional-control-flow/)

### 7 · 浮点 / SIMD

- `%xmm0`–`%xmm15`（及 `%ymm`/`%zmm` 扩展）与 GPR 分开；caller/callee 保存规则有专表。  
- `float`/`double` 在向量槽中的格式与 **16B 对齐** 要求影响栈对齐硬性规则。  
→ [§3.11](../../chapter-03-machine-level-programs/notes/section-3.11-浮点代码.md)

### 8 · 动态链接（`.so`）

- **PLT / GOT**：间接跳进共享库函数；首次常走解析。  
- 符号可见性、版本、重定位规则保证 **多年后旧二进制仍能链新 glibc**（ABI 稳定）。  
→ [Ch7](../../chapter-07-linking/) · 上文「例 6」

### 9 · 与 ARM32 AAPCS 同构、实现不同

| 大类 | System V x86-64 | AAPCS ARM32 |
|------|-----------------|-------------|
| 整型传参 | `%rdi`… 共 6 | `r0`–`r3` |
| callee-save | `%rbx,%rbp,%r12–%r15` | `r4`–`r11`（约） |
| 栈对齐 | **16B**（call 前） | 常 **8B** |
| 结构体 / 段 / syscall | 都有明确条款 | 同样有，寄存器名不同 |

→ [Smith §13.5](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-13-subroutines-stacks/notes/section-13-5-apcs.md) · 下文实战例 1–6

**一句话：** ABI = 全链路二进制协议；**不只是 `sizeof`。**

---

## 7. 遵守 ABI 的实战例子（ARM32 / AArch64 · C/汇编）

> 嵌入式 + HFT：汇编与 C/Rust 互调、跨 .so、进内核，**全靠同一套二进制协议**。  
> ARM32 详规 → [Smith Ch13 AAPCS](../../../19-ARM64-Architecture/arm32-smith-assembly/chapter-13-subroutines-stacks/notes/section-13-5-apcs.md)（旧文献也称 **ATPCS**）。

### 例 1 · ARM32 传参（AAPCS / ATPCS）

**规则：** 前 4 个整型参数 → `r0–r3`；第 5 个起压栈；返回值 → `r0`；用到的 **callee-save**（如 `r4–r11`）须保存/恢复。

```c
int add(int a, int b, int c, int d, int e)
{
    return a + b + c + d + e;
}
```

```asm
add:
    push    {r4, lr}          ; 示范：占用 callee-save 则须保存（本例未真用 r4）
    add     r0, r0, r1        ; a+b
    add     r0, r0, r2        ; +c
    add     r0, r0, r3        ; +d
    ldr     r1, [sp, #8]      ; 第 5 参 e：入口在栈上；push 后偏移 +8
    add     r0, r0, r1
    pop     {r4, pc}          ; 恢复 + 返回（返回值在 r0）
```

**合规点：** 寄存器传参、多余参数在栈、callee-save、返回 `r0` → 可与 GCC 编的 `.o` 互链。

### 例 2 · AArch64 传参（HFT / Linux）

**规则：** 整型参数 `x0–x7`；返回 `x0`；浮点常 `v0–v7`。

```c
long calc(long x, long y) { return x * y; }
```

```asm
calc:
    mul     x0, x0, x1        ; 入参即 x0/x1，积回 x0
    ret
```

→ C / Rust / 手写汇编底层可无缝互调（同 AAPCS64）。

### 例 3 · 结构体内存对齐（Linux LP64 ABI · 第 2 项）

**规则：** 成员按自身对齐；结构体整体对齐 = 最大成员对齐 → 自动 **padding**。

```c
struct Test {
    char a;   /* offset 0 */
    long b;   /* LP64：long 8 字节对齐 → a 后补 7；b @ offset 8 */
};
/* sizeof == 16；数组下一元素仍按 8 对齐起步 */
```

| ABI | 典型布局直觉 |
|-----|----------------|
| **Linux LP64** | `a` + 7 pad + `b`(8) → **16** |
| **ILP32** | `long` 常 4 字节、按 4 对齐 → pad/`sizeof` **更小** |
| **Windows LLP64** | `long` 常 4 字节 → 与 Linux x64 **同 CPU 仍可能布局不同** |

C 与汇编对 **`offsetof`** 必须一致；强行改对齐或不按同一 ABI 互传结构体 → 读错偏移、崩。

### 例 4 · C++ 调 C：`extern "C"` 对齐 **C ABI 符号**

C++ **名字修饰** 会弄出另一套符号名 → 链接对不上。  
`extern "C"` 强制按 **C 链接/符号规则**（仍要同平台调用约定）。

```c
/* test.h — 被 C++ include 时 */
#ifdef __cplusplus
extern "C"
#endif
int func(int val);
```

```c
/* C 实现 */
int func(int val) { return val * 2; }
```

```cpp
#include "test.h"
int main() { return func(10); }  /* 符号匹配，能链接 */
```

### 例 5 · Linux 系统调用 ABI（用户态 → 内核）

**AArch64 Linux：** 调用号 → **`x8`**；参数 → **`x0–x5`**；执行 **`svc #0`**。

```asm
    mov     x8, #63           ; read 的 syscall 号
    mov     x0, #0            ; fd = stdin
    adrp    x1, buf
    add     x1, x1, :lo12:buf
    mov     x2, #16
    svc     #0                ; 进内核；寄存器放错则调用失败
```

### 例 6 · 动态库 `.so` 的 ABI 稳定

glibc 长期保证 **`printf` 等符号的调用约定与布局** 不变 → 2018 年编的旧程序，2026 新系统上仍常能直接跑，核心就是 **ABI 不崩**。

### 反面：不遵守会怎样

ARM32 汇编若 **私自用 `r4` 传第一个参数**、又不保存恢复 `r4`：

- 与 C 互调时参数错位  
- 调用方认为「`r4` 没变」→ 变量被篡改 → 常见段错误 / 无法链接正确行为  

---

## 8. HFT / 跨平台

1. **`sizeof(int/long)` 为何变** — 目标 **ABI** 写死类型模型，不是编译器随意  
2. **二进制协议** — `int32_t`/`int64_t` + 显式 endian；**禁止** 本地 `struct` / `long` 上 wire  
3. **C++ / Rust / DPDK / 内核** — 共享边界须 **同一 ABI**（HFT 常锁 Linux x64 LP64 + System V；ARM 板子锁 AAPCS/AAPCS64）  
4. **手写汇编** — 无编译器兜底时 **自己守** 传参/callee-save（见上节例 1–2、例 5）

```c
#include <stdint.h>   /* 固定宽度 */
/* wire：显式 endian；别 struct 裸 send */
```

---

## 9. 极简总结

**ABI = 二进制世界里的底层通信协议** — 类型宽度只是入口；真正日常踩坑的是 **传参/返回、caller/callee-save、16B 栈对齐、结构体 padding、ELF 段、syscall、PLT/GOT**。  
**API = 源码世界里的开发协议** — 人与编译器之间的接口声明。  
**实战：** C↔汇编、跨 `.so`、进内核 — 条款写在 System V / AAPCS / 内核 syscall ABI 里。

---

## 口述巩固 · 自测

1. 为什么说 ABI 是「协议」？`sizeof` 只是其中哪一层？  
2. System V 前 6 个整型参数寄存器？浮点用哪组？  
3. caller-saved vs callee-saved 各举两个；`call` 前 `%rsp` 要对齐到几字节？  
4. Linux x86-64 `syscall`：调用号在哪？第 4 个参数为何是 `%r10`？  
5. `.text` / `.rodata` / `.data` / `.bss` 各放什么？  
6. AAPCS 和 System V 各举一条「同构不同寄存器」的条款。  
7. `extern "C"` 解决的是 ABI 的哪一层问题？  
8. 把 `struct Message` 直接 `send()` 会踩 ABI 的哪几条规则？  
9. `struct {char a; long b;}` 在 Linux LP64 下为何常是 16 字节？换 ABI 会怎样？

---

← [Ch2 导读](../README.md) · [§2.1.2 数据大小](./section-2.1.2-数据大小与sizeof.md) · [01 code](../../code/)
