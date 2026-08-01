## 3. 进程地址空间与内存布局

> 加载后 — **text / data / BSS / stack** + **argv / envp**

---

### 一、经典段

| 段 | 内容 |
|----|------|
| **Text（代码）** | 只读、可执行指令 |
| **Data** | **已初始化** 全局/静态变量 |
| **BSS** | **未初始化** 全局/静态 — 通常零填充 |
| **Stack** | 局部变量、调用链 |

→ 寻址与页表：[Ch 2](../chapter-02-memory-addressing/) · VMA：[Ch 9](../chapter-09-process-address-space/)

---

### 二、argv 与 envp

**`execve(path, argv, envp)`** 传入：

- **命令行参数** `argv[]`  
- **环境变量** `envp[]`  

内核将它们放在 **用户栈最高地址附近**（栈 **底部** / 向低地址生长前的顶端），新程序 `_start` / libc 从此解析。

---

### 三、灵活内存区布局 (Flexible Layout, 2.6.9+)

**传统 80x86：**

- **mmap 区**（共享库等）常从 **`0x40000000`**（用户空间 **1/3** 处）固定起  

**灵活布局：**

- 若可由 **`RLIMIT_STACK`** 确定栈 **上限**  
- **mmap 区** 放在 **靠近栈底的高地址**，向 **低地址** 生长  
- **堆 (heap)** 获得 **更大连续扩展空间**  

→ 堆/brk：[Ch 9 section-6](../chapter-09-process-address-space/notes/section-6-写时复制与堆.md)

> **Modern 对照：** ASLR 进一步随机化 mmap/stack 基址 — ULK 2.6 为布局 **灵活性** 奠基。

---

← [2. 凭证](./section-2-进程凭证与能力.md) · 下一节 [4. ptrace](./section-4-执行跟踪ptrace.md)
