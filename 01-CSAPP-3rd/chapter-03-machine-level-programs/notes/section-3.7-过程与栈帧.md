## 3.7 过程（函数调用与栈帧）

### 3.7.1 运行时栈

每个函数调用在栈上形成 **栈帧 (stack frame)**：

```
高地址
  调用者栈帧
  返回地址          ← call 压入
  保存的 %rbp（可选）
  局部变量 / 临时空间
  参数构造区（若 >6 个参数）
低地址 ← %rsp
```

- **栈溢出 (stack overflow)** — 递归过深或 `alloca` 过大

### 3.7.2 转移控制

- `call label` — push `%rip` 后继，jump
- `ret` — pop 到 PC（**ret 地址被篡改 = 控制流劫持**）

### 3.7.3 数据传送 — **System V AMD64 ABI**（Linux x86-64）

> **全景地图**（传参以外：栈对齐、ELF、syscall、SIMD、`.so`）→ [Ch2 · ABI §6](../../chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md#6-x86-64-system-v-abi-全景linux--除类型占几字节以外)

**整数/指针参数（前 6 个）：**

| 顺序 | 寄存器 |
|------|--------|
| 1–6 | `%rdi, %rsi, %rdx, %rcx, %r8, %r9` |
| 7+ | 栈上（从右到左压参的旧约定残留于栈布局） |

**浮点参数：** `%xmm0`–`%xmm7`（与 GPR 分开）

**返回值：** `%rax`（及 `%rdx` 若 128 位）；浮点 → `%xmm0`

**被调用者保存 (callee-saved)：** `%rbx, %rbp, %r12–%r15`  
**调用者保存 (caller-saved)：** `%rax, %rcx, %rdx, %rsi, %rdi, %r8–%r11`

**栈：** 向低地址增长；**`call` 前 `%rsp` 须 16 字节对齐**（SSE）。

**HFT：** C 调 Rust / 手写汇编 / `ioctl` 包装 — **必须遵守同一 ABI**；Windows 是另一套约定。

**低延迟抓手（传参）：** 前 6 个在寄存器里几乎「免费」；第 7 个起进栈。交易热路径 API 尽量 **把参数压在 6 个以内**（或打包进一个指针/结构体指针），少压栈。

### 3.7.4–3.7.5 局部存储：栈 vs 寄存器


- 小局部变量、频繁使用 → 寄存器分配（`-O`）
- 取地址 `&x`、大结构、变长数组 → **必须在栈**
- **寄存器溢出 (spill)** — 寄存器不够时写栈，增加 load/store

### 3.7.6 递归

- 每次 `call` 新栈帧；尾递归可被优化成 **循环**（`-O2`）

### 3.7.7 `inline`（C/C++ 语法，不是汇编关键字）

**`inline` 不是 x86 汇编里的助记符**，而是 **C/C++** 给编译器的提示：尽量把函数体 **直接嵌到调用点**，从而少做一次真正的过程调用。

| 普通调用时机器在干啥 | `inline` 成功后 |
|----------------------|-----------------|
| 生成 **`call`**（压返回地址、跳转） | **没有** 那条 `call` |
| 可能建栈帧、参数进栈/寄存器再约定传递 | 加减等指令 **直接出现在调用处** |
| **`ret`** 退栈、跳回 | 控制流自然接着往下走 |

**x86 / HFT 例子：** 算订单价差的小函数加上 `inline` 后，编译器常不再单独生成 `call`，而是把里面的加减嵌进热路径 — 省压栈/跳转/退栈；指令更连续，也方便后续在同一基本块里做寄存器分配与强度削减（数据少往栈上倒）。每少一轮流水线扰动，都可能多抢一点成交时间。

```c
static inline int64_t spread(int64_t ask, int64_t bid) {
    return ask - bid;   /* 希望热路径里变成几条算术，而不是 call */
}
```

注意：`inline` 是 **请求**；是否真内联看优化级别、函数是否过大、是否跨翻译单元可见。要硬保证可用 `always_inline` / 同 `.c`/`header` 可见（→ [Ch5 §5.5](../../chapter-05-optimizing-performance/notes/section-5.5-减少过程调用.md)）。用 `gcc -S` / `objdump` 核对有没有 `call`。

**HFT 实践：**

- 热路径 **禁止深递归**；订单簿遍历用循环 + 显式栈/arena  
- 热路径小函数优先 **`inline`** — 对照 [Ch4 流水线/控制冒险](../../chapter-04-processor-architecture/README.md)  
- `perf` 看 `__stack_chk_fail` — stack canary 触发说明栈破坏  
- 强度削减（`*4`→移位等）→ [§3.5](./section-3.5-算术与逻辑操作.md) · [Ch5 §5.1](../../chapter-05-optimizing-performance/notes/section-5.1-优化编译器的能力和局限性.md)

→ 链接与符号：[Ch 7](../../chapter-07-linking/)

---

← [本章导读](../README.md)
