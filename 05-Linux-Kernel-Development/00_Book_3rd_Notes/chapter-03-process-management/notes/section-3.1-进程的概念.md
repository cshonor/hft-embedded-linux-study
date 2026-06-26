## ① 进程的概念 · The Process

**进程** = 处于 **执行期** 的程序 + **相关资源** 的集合：

| 资源示例 | 说明 |
|----------|------|
| 打开的文件 | fd 表 |
| 挂起的信号 | 待递送信号 |
| 处理器状态 | 寄存器、PC |
| 内存地址空间 | 虚拟内存布局 |
| 一个或多个执行线程 | Linux 中仍属「进程」模型 |

#### 典型创建路径（Unix 两步）

```
fork()  ──► 复制现有进程（子进程）
   │
   └──► exec() 族 ──► 加载新可执行文件，替换映像
```

| 调用 | 作用 |
|------|------|
| **`fork()`** | 复制进程 — 父子并发 |
| **`exec*()`** | 换程序 — 常接在 `fork` 之后 |

**HFT 对照：** 热路径网关多用 **线程池 + `clone`/`pthread`**，极少 **每连接 `fork`**（页表/COW 仍懂成本即可）。

→ [01-CSAPP Ch8 fork/exec](../../../../01-CSAPP-3rd/chapter-08-exceptional-control-flow/) · [Ch9 COW](../../../../01-CSAPP-3rd/chapter-09-virtual-memory/notes/section-9.8-内存映射mmap.md) · [08-TLPI 进程章](../../../../08-The-Linux-Programming-Interface/)

---
