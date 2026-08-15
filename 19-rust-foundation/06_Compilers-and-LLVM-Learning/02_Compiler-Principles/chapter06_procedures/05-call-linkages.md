# 第 6 章 · 过程抽象 · §5 标准链接约定

← [本章目录](./README.md) · 上一节：[04-parameter-passing.md](./04-parameter-passing.md) · 下一节：[06-memory-and-gc.md](./06-memory-and-gc.md)

---

**链接约定（Calling Convention / Linkage）** — 不同过程（甚至不同语言）能互调的**契约**。

一次完整调用 = **四段代码**协作：

---

## 1. Precall（调用前序列）— 调用者

| 职责 |
|------|
| 计算/排列**实参** |
| 为被调者准备环境（栈空间、寄存器传参） |
| 保存 caller-saved 寄存器（若需要） |
| 发出 **call** 指令 |

---

## 2. Prologue（序言）— 被调用者入口

| 职责 |
|------|
| 保存 **FP**，建立新 **FP/SP** |
| **分配 AR**（减 SP） |
| 保存 callee-saved 寄存器 |
| （可选）设置 static/access link |

---

## 3. Epilogue（结语）— 被调用者出口

| 职责 |
|------|
| 放置**返回值**到约定位置 |
| 恢复 callee-saved |
| 释放 AR、恢复 FP/SP |
| **ret** |

---

## 4. Postreturn（返回后序列）— 调用者

| 职责 |
|------|
| 从约定位置取返回值 |
| **清理栈参数**（如 x86 cdecl） |
| 恢复 caller 上下文 |

---

## 常见 ABI 实例

| ABI | 平台 |
|-----|------|
| **System V AMD64** | Linux / macOS Rust 默认 |
| **Microsoft x64** | Windows |
| **AAPCS** | ARM |

**Rust FFI**：`extern "C"` → C ABI；错误 ABI = 静默崩溃。

→ RFR 第 13 章工具链 · `extern` 块

---

## 与 HFT / 性能

| 因素 | 影响 |
|------|------|
| **寄存器传参 vs 栈传参** | 延迟 |
| **Prologue/Epilogue 大小** | 小函数内联可消除 |
| **调用边界** | 阻止跨 call 优化 |

**`#[inline]` / LTO**：编译器合并或消除调用序列。

---

## 与 ch11 后端

**指令选择 + 寄存器分配** 在 ABI 约束下生成 precall/prologue/epilogue 具体机器码。
