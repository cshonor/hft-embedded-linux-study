## 8.6–8.8 非本地跳转、工具与小结

### 8.6 非本地跳转

```c
#include <setjmp.h>
jmp_buf env;
if (setjmp(env) == 0) {
    // 正常路径
} else {
    // longjmp(env, 1) 跳回此处
}
longjmp(env, 1);
```

- **不还原栈展开** — 与 C++ **异常、RAII** 不兼容；**跳过析构**
- 用途：深层错误快速回退（老代码）；现代 C++ 用 **异常** 或 **Result 类型**

**HFT：** 新代码 **避免 setjmp**；协程/状态机用显式枚举更清晰。

### 8.7 操作进程的工具

| 工具 | 用途 |
|------|------|
| `ps` | 进程列表 |
| `top`/`htop` | 实时 CPU/内存 |
| `pmap` | 地址空间映射 |
| `strace` | **跟踪 syscall** — 查意外阻塞 |
| `/proc/<pid>/` | 状态、fd、maps |

```bash
strace -c ./strategy    # syscall 统计
strace -e trace=network ./gateway
```

**HFT：** `strace` **开销巨大** — 只在测试环境查「谁在 syscall」；生产用 `perf`/`bpftrace`。

### 8.8 小结（原书）

- **ECF** 贯穿硬件→内核→进程→信号
- **进程** 是 OS 核心抽象；**shell = fork/exec/wait + 信号作业控制**
- 为 **VM (Ch9)** 和 **并发 (Ch12)** 铺路

→ [Ch 9 虚拟内存](../chapter-09-virtual-memory/) · [Ch 12 并发](../chapter-12-并发编程.md)

---

← [本章导读](../README.md)
