## 10.10–10.12 标准 I/O 与选型

### 10.10 标准 I/O (stdio)

```c
FILE *fopen(...);
size_t fread/fwrite(...);
char *fgets(...);
int fprintf/scanf(...);
```

- **`FILE*`** 带 **应用层缓冲** — 减少 syscall，但 **与 fd 层混用要小心**（`fflush`、重复缓冲）
- **线程安全** — `flockfile`；多线程热路径更倾向 **裸 fd + 自管缓冲**

### 10.11 综合：该用哪些 I/O？

| 方式 | 优点 | 缺点 | HFT |
|------|------|------|-----|
| **Unix I/O** (`read/write`) | 可控、配合 `epoll`/非阻塞 | 需自处理短计数 | **网络/管道主选** |
| **Rio** | 健壮读写模板 | 教学包，非极致性能 | 学习/工具程序 |
| **标准 I/O** | 格式化、方便 | 缓冲与 fd 混用坑 | **日志、配置** |
| **mmap** | 大文件顺序访问、零拷贝感 | 缺页、同步 | **replay 文件**（→ [Ch 9](../chapter-09-virtual-memory/notes/section-9.8-内存映射mmap.md)） |

**决策树（简化）：**

```
大文件只读顺序？ → mmap
需要 printf 格式？ → stdio（冷路径）
socket/管道/精细控制？ → Unix I/O + 自缓冲
延迟极致？ → 绕过内核栈（DPDK）仍理解 fd 语义
```

### 10.12 小结（原书）

- fd 抽象统一 I/O；**短计数** 是常态不是异常
- Rio 展示 **正确读写模式**
- 为 **Ch11 网络**（socket = fd）和 **Ch12 并发**（多线程共享 fd）打基础

→ [Ch 11 网络编程](../../chapter-11-network-programming/) · [09-UNP](../../../09-UNP-Vol1/)

---

← [本章导读](../README.md)
