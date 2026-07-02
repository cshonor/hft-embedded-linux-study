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

**整数/指针参数（前 6 个）：**

| 顺序 | 寄存器 |
|------|--------|
| 1–6 | `%rdi, %rsi, %rdx, %rcx, %r8, %r9` |
| 7+ | 栈上（从右到左压参的旧约定残留于栈布局） |

**返回值：** `%rax`（及 `%rdx` 若 128 位）

**被调用者保存 (callee-saved)：** `%rbx, %rbp, %r12–%r15`  
**调用者保存 (caller-saved)：** `%rax, %rcx, %rdx, %rsi, %rdi, %r8–%r11`

**HFT：** C 调 Rust / 手写汇编 / `ioctl` 包装 — **必须遵守同一 ABI**；Windows 是另一套约定。

→ ABI 总览：[Ch2 §2.1.2 ABI](../chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md)

### 3.7.4–3.7.5 局部存储：栈 vs 寄存器

- 小局部变量、频繁使用 → 寄存器分配（`-O`）
- 取地址 `&x`、大结构、变长数组 → **必须在栈**
- **寄存器溢出 (spill)** — 寄存器不够时写栈，增加 load/store

### 3.7.6 递归

- 每次 `call` 新栈帧；尾递归可被优化成 **循环**（`-O2`）

**HFT 实践：**

- 热路径 **禁止深递归**；订单簿遍历用循环 + 显式栈/arena
- `perf` 看 `__stack_chk_fail` — stack canary 触发说明栈破坏

→ 链接与符号：[Ch 7](../../chapter-07-linking/)

---

← [本章导读](../README.md)
