## 2. POSIX API 与系统调用

> 用户代码通常调 **API**，不是直接调 **syscall**

---

### 一、API ≠ 系统调用

| 层次 | 说明 |
|------|------|
| **POSIX 标准** | 认证的是 **API**（函数接口），不是具体内核 syscall 实现 |
| **`libc`** | 在用户态实现 API，内部再触发 syscall |

**例：`malloc` / `free`**

```
malloc() / free()     ← libc API（堆算法、缓存）
    ↓
brk() / mmap()        ← 真正的系统调用（扩/缩堆、大块映射）
    ↓
sys_brk() / sys_mmap() ← 内核服务例程
```

→ 堆与 VMA：[Ch 9 section-6](../../chapter-09-process-address-space/notes/section-6-写时复制与堆.md)

---

### 二、封装例程 (Wrapper Routines)

glibc 为每个 syscall 提供薄封装（如 `read()` → `syscall(SYS_read, ...)`）。

**返回值约定差异：**

| 层 | 成功 | 失败 |
|----|------|------|
| **内核 `sys_*()`** | 非负整数或 0 | **负 errno 值**（如 `-EFAULT`） |
| **libc 封装** | 原值返回 | 取绝对值写入 **`errno`**，向用户返回 **-1** |

用户态应检查 **返回值 + `errno`**，而非假设内核负返回值直接冒泡。

→ 用户态详述：[08 TLPI](../../../06-The-Linux-Programming-Interface/)

---

### 三、为何多一层 API

- **可移植** — 同一 POSIX API，不同 OS 不同 syscall 号/语义  
- **策略** — `stdio` 缓冲、`malloc` arena 等在 libc 完成  
- **兼容** — 老程序链接新 libc 无需改 syscall 细节  

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 分派表](./section-3-分派表与服务例程.md)
