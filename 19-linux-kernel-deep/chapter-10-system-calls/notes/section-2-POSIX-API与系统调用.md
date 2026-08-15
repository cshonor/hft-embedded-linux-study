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

→ 用户态详述：[08 TLPI](../../../03-linux-userspace-api/)

---

### 三、为何多一层 API

- **可移植** — 同一 POSIX API，不同 OS 不同 syscall 号/语义  
- **策略** — `stdio` 缓冲、`malloc` arena 等在 libc 完成  
- **兼容** — 老程序链接新 libc 无需改 syscall 细节  

### 常见陷阱

1. 混淆 POSIX API 和 syscall——`malloc` 是 POSIX API（glibc 实现），底层调 `brk`/`mmap` syscall
2. 以为每个 libc 函数对应一个 syscall——`printf` → `write`，`fork` → `clone`，映射不是一一对应
3. 以为 syscall 一定经过 libc——可以直接内联汇编 `syscall` 指令绕过 libc（如 Go runtime）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** POSIX API、libc 函数、syscall 三者的关系？

<details><summary>答案</summary>

POSIX API 是标准接口规范（如 `open`/`read`/`write`/`fork`）。libc（glibc/musl）实现 POSIX API，内部调用 syscall。不是一一对应：`fork()` → `clone()` syscall，`exit()` → `exit_group()` syscall，`printf()` → `write()` syscall（多层封装）。可直接内联汇编调 `syscall` 指令绕过 libc（Go runtime、Crystal 语言这么做，减少 libc 依赖和开销）。

</details>

**Q2.** 为什么 Go runtime 不用 libc 调 syscall？

<details><summary>答案</summary>

① 避免 libc 的信号处理和 TLS 冲突（Go 有自己的 goroutine 调度）。② 减少 libc 封装开销（虽然很小）。③ 独立控制 syscall 行为（如 Go 的非阻塞 I/O 直接用 `epoll` + `nonblock`）。Go 用 `runtime/sys_linux_amd64.s` 中的汇编直接 `syscall` 指令。缺点：无法利用 vDSO（Go 自己实现了 vDSO 解析）。

</details>

**Q3.** HFT 中如何测量单次 syscall 的开销？

<details><summary>答案</summary>

```c
// 用 RDTSC 测量
uint64_t t1 = rdtsc();
syscall(SYS_getpid);  // 最简单的 syscall
uint64_t t2 = rdtsc();
printf("getpid: %lu cycles (~%lu ns)\n", t2-t1, (t2-t1)/3000);  // 3GHz CPU
```
典型值：`getpid` ~150ns，`read` ~200ns，`epoll_wait` ~300ns（无事件），`mmap` ~1-5us。

</details>

</details>

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 分派表](./section-3-分派表与服务例程.md)
> ↔ [LKD Ch05 §5.2 系统调用基础](../../../05-linux-kernel/chapter-05-system-calls/notes/section-5.2-系统调用基础.md)
