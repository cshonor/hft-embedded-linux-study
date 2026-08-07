## 9.5 虚拟内存作为保护工具

> **Ch9 §9.5** · [章导读](../README.md) · 上节 [§9.4 ←](./section-9.4-虚拟内存作为内存管理工具.md) · 下节 [§9.6 →](./section-9.6-地址翻译.md)

---

← [本章导读](../README.md)

---

### PTE 权限位与保护机制

- **PTE 权限位：** `SUP`（内核专用）/ `R` / `W` / `X`
- **检查时机：** 每次 MMU 地址翻译时自动检查，无需软件参与
- **违规 → page fault** — CPU 触发异常，OS 终止进程或发送信号

| 权限位 | 含义 | 违规后果 |
|--------|------|----------|
| SUP=1 | 仅内核可访问 | 用户态访问 → SIGSEGV |
| R=0 | 不可读 | 读操作 → SIGSEGV |
| W=0 | 不可写 | 写操作 → SIGSEGV（含 COW 触发） |
| X=0 | 不可执行 | 执行 → SIGSEGV（DEP/NX） |

**HFT：** 代码段设 R+X，数据段设 R+W（不执行），栈不可执行（防溢出利用）。

### 常见陷阱
1. **保护检查是 PTE 硬件位，不是软件检查** — MMU 在翻译时自动验证，不需要 OS 逐次审查
2. **用户态访问内核页 → SIGSEGV，不是 silently fail** — SUP 位违规直接触发异常终止进程
3. **权限粒度是页级（4KB），不是字节级** — 同一页内所有字节权限相同；细粒度保护需要特殊机制（MPK 等）

### 自测题

<details>
<summary>Q1: PTE 的 SUP 位是什么作用？</summary>

SUP=1 表示该页仅内核态可访问。用户态访问 SUP=1 的页会触发 page fault → SIGSEGV。

</details>

<details>
<summary>Q2: DEP（数据执行保护）依赖哪个 PTE 位？</summary>

X（可执行）位。数据页设 X=0，代码页设 X=1。尝试执行数据页（如栈溢出 shellcode）触发 SIGSEGV。

</details>

<details>
<summary>Q3: 为什么写只读页（W=0）有时不报错而是触发 COW？</summary>

fork 后共享页标记为只读。写时 MMU 检测到 W=0 触发 fault，OS 判断是 COW 场景，分配新物理页后重启写操作，而非终止进程。

</details>

<details>
<summary>Q4: 内存保护粒度是什么？能否实现字节级保护？</summary>

页级（4KB），同一页所有字节权限相同。字节级保护需要硬件扩展（如 Intel MPK 提供 16 个 protection key），但仍有页级限制。

</details>

---

← [§9.4 ←](./section-9.4-虚拟内存作为内存管理工具.md) · [本章导读](../README.md) · [§9.6 →](./section-9.6-地址翻译.md)
