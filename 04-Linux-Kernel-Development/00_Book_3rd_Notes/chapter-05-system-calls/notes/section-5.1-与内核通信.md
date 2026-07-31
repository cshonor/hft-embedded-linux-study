## ① 与内核通信 · Communicating with the Kernel

整体链路：**用户进程（用户态）↔ 内核（内核态）**。  
`open` / `read` / `write` / `fork` / `dup` 全部建立在系统调用之上 — 与 [§3.8 fd / struct file / inode](../../chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md) 直接衔接。

---

### 一句话先钉死：libc ≠ 系统调用

**系统调用**是内核提供给用户程序的底层原始接口（靠 CPU 指令陷入内核）；  
**libc** 是运行在**用户态**的标准 C 库，最核心工作之一是：**封装系统调用**。

```
你的 C 代码 → libc 函数 → 系统调用（syscall 指令）→ Linux 内核
```

---

### 核心问题：两种特权级

| 模式 | 谁在跑 | 权限 |
|------|--------|------|
| **用户态 User Mode** | 应用程序 | 受限：不能直接操作硬件、不能访问内核内存、不能改页表 |
| **内核态 Kernel Mode** | 操作系统内核 | 完整硬件权限 |

应用想要：读写磁盘、创建进程、操作文件、分配内存 — **自己做不到，必须请求内核帮忙**。

这套「用户态主动陷入内核，请求内核服务」的机制，就是 **系统调用（System Call）**。

---

### 典型误读：`write()` 是不是系统调用？

```c
#include <unistd.h>
int main(void) {
    write(1, "hi\n", 3);
}
```

| 说法 | 对错 |
|------|------|
| 「`write()` 就是系统调用」 | ❌ |
| `write()` 是 **libc 提供的用户态函数** | ✅ |
| 真正的系统调用没有「C 函数名」，只有 **数字编号**（x86_64 上 `__NR_write` 常为 `1`） | ✅ |

#### libc `write` 内部（极简示意，非 glibc 原文）

```asm
write:
    ; 把参数放到寄存器（约定因架构而异）
    mov     rax, 1          ; 系统调用号 __NR_write = 1
    syscall                 ; 触发 CPU 指令，陷入内核
    cmp     rax, -4096
    ja      .Lerror         ; 处理内核返回的负数错误码
    ret
.Lerror:
    neg     rax
    mov     errno, rax      ; 把错误写入 errno（libc 变量）
    mov     rax, -1
    ret
```

| 谁 | 干什么 |
|----|--------|
| **libc `write`** | 整理参数 → 执行 `syscall` → 把内核负错误码转成 **返回 -1 + 设 `errno`** |
| **内核 `sys_write`** | `syscall` 触发后真正干活的代码 |

---

### 三组极易混淆的概念

#### ① 系统调用（System Call）

内核对外暴露的 **最小原生接口**：

- 没有函数名，只有 **系统调用号**；
- 必须靠 `syscall` / `int 0x80` / `svc` 等 CPU 指令切到内核态；
- 用户程序 **裸写汇编、不链接 libc**，也能直接发起系统调用。

→ 演示见 [`code/write_raw_syscall.s`](../code/write_raw_syscall.s)（无 libc）。

这直接证明：**系统调用独立于 libc 存在；libc 只是包装器。**

#### ② libc（Linux 上主流是 glibc）

用户态库，功能大致两类：

| 类型 | 例子 |
|------|------|
| **封装系统调用** | `open` / `read` / `write` / `fork` / `close` / `mmap` … |
| **纯用户态，不碰内核** | `strlen` / `strcmp` / `memcpy` / `sqrt` / `printf` 的格式化算法 |

> 很多 libc 函数 **根本没有对应的系统调用**。例如 `strcpy`：只在用户内存操作，不会陷入内核。

#### ③ 高级 I/O（带缓冲区，仍属 libc）

`fopen` / `fread` / `fwrite` / `printf` — 同样是 libc，内部多一层 **用户态缓冲**：

```
printf → libc 缓冲 →（满了或 flush）→ libc write() 封装 → syscall → 内核 sys_write
```

| 层次 | 谁 |
|------|-----|
| **用户写的** | `printf` → 往往经 libc 缓冲，底层才到 `write` |
| **实际跨界** | **`syscall` 指令** |
| **内核** | `sys_write()` 等 |

---

### 最容易踩的 3 个误区

#### 误区 1：`open()` = 系统调用

C 代码里的 `open()` 是 **glibc 函数**。  
现代 Linux 上，glibc 内部常发起 **`openat` 系统调用**（老 `open` 多被兼容封装）。  
`strace ./a.out` 常见：源码写 `open`，内核侧显示 `openat`。

#### 误区 2：没有 libc 就无法发起系统调用

纯汇编 `syscall` 即可（见上方 demo）。  
只是手动管寄存器和错误码很麻烦，所以几乎所有程序都链接 libc。

#### 误区 3：系统调用返回值 == libc 函数返回值

| 侧 | 约定 |
|----|------|
| **内核** | 负数 = 错误（如 `-EINTR`） |
| **libc** | 收到负码 → `return -1` 且 **`errno = 正错误码`** |

**`errno` 是 libc 的线程局部变量；内核没有 `errno`，也不知道它是什么。**  
转换工作完全由 libc 做。

---

### 和 fd 模型串起来

```c
int fd = open("a.txt", O_RDWR);
```

1. 你的代码调用 `open()` — **glibc 函数，用户态**
2. glibc 准备参数，`rax` 写入系统调用号，执行 `syscall`
3. CPU 切内核态，执行内核 `sys_openat` — **系统调用处理函数**
4. 内核：查 inode、创建 `struct file`、在 `task_struct->files` 分配 fd
5. 内核把 fd 数字放入 `rax`，`sysret` 回用户态
6. glibc 拿到返回值，原样（或按错误约定转换后）返回给你的代码

→ 细节：[§3.8](../../chapter-03-process-management/notes/section-3.8-身份PID与资源FD.md) · 本章 README「串联 open」

---

### 不是所有系统都叫 glibc

| 环境 | 常见 C 库 |
|------|-----------|
| Linux 桌面/服务器 | **glibc** |
| 嵌入式 / Alpine | **musl**（更轻） |
| Windows | msvcrt / UCRT 等 |

实现不同，作用一致：封装系统调用 + 提供标准 C 函数。

> 别和 **GNU C** 搞混：glibc = 用户态库；GNU C = **GCC 语法扩展**（内核 `-std=gnu11`）。  
> 内核用 GNU C 写，但 **不链接 glibc**。详见 [Ch2 §2.4](../../chapter-02-getting-started/notes/section-2.4-内核开发的特点.md)。

---

### 背诵版

1. **系统调用**：内核底层服务；靠 CPU 指令切特权级；只有数字编号。  
2. **libc**：用户态 C 库；封装 syscall、整理参数、做 `errno` 转换。  
3. **libc 依赖系统调用；系统调用不依赖 libc。**  
4. 日常 C 里写的 `open/read/write` **全部是 libc 函数名**，不是「系统调用本身」。

---

### 对比 demo（Linux x86_64）

| 文件 | 说明 |
|------|------|
| [`code/write_libc_demo.c`](../code/write_libc_demo.c) | 经 libc `write` |
| [`code/write_raw_syscall.s`](../code/write_raw_syscall.s) | **不链接 libc**，直接 `syscall` |

```bash
# ① libc
cc -Wall -o write_libc_demo write_libc_demo.c && ./write_libc_demo
strace -e write ./write_libc_demo

# ② 无 libc（需 Linux x86_64 工具链）
as --64 -o write_raw_syscall.o write_raw_syscall.s
ld -o write_raw_syscall write_raw_syscall.o
./write_raw_syscall
strace -e write,exit ./write_raw_syscall
```

---

### 其他通信方式对比（§5.6 会用到）

| 方式 | 特点 |
|------|------|
| **系统调用** | 标准原生接口，开销相对可控 |
| **ioctl** | 设备文件上的扩展命令 |
| **netlink** | 用户态 ↔ 内核态 **双向消息** |
| **procfs / sysfs** | 文件形式读写内核参数 |

用户态 **不能** 直接跳转内核函数地址，必须走系统调用陷阱。

---

### Unix 设计原则

> **提供机制，而不是策略**（mechanism, not policy）

| 机制 | 策略 |
|------|------|
| 内核提供 **抽象能力**（读 fd、映射内存） | **用户程序决定** 何时读、读多少、怎么用 |

**HFT：** 热路径倾向 **批量 I/O、`mmap`、用户态轮询/DPDK** — 本质是在 **减少机制调用次数**（少进内核，而不是「少写几个 libc 函数名」）。

→ [03 SysPerf §3.2](../../../../15-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.2-内核基础与核心概念.md) · [Ch 1](../../chapter-01-intro/) · 下一节 [§5.2](./section-5.2-系统调用基础.md)

---
