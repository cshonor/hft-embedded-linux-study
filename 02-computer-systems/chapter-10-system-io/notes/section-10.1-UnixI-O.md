## 10.1 Unix I/O

> **Ch10 §10.1** · [章导读](../README.md) · 上节 — · 下节 [§10.2 →](./section-10.2-文件.md)

---

- **一切皆文件** — 普通文件、目录、设备、**socket**、管道
- **文件描述符 (fd)** — 小整数，进程 **fd 表** 中的索引

| fd | 默认 |
|----|------|
| 0 | stdin |
| 1 | stdout |
| 2 | stderr |

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. Unix I/O 模型的核心思想是什么？</summary>

Unix I/O 的核心：**一切皆文件**。所有 I/O 设备（磁盘、网络、终端、管道）都抽象为文件，用统一的 `open/read/write/close` 接口操作。每个打开的文件用一个非负整数 **fd(file descriptor)** 标识。

fd 0=stdin, 1=stdout, 2=stderr（预打开）。`open` 返回最小可用 fd。这个统一接口让程序可以用同一套代码处理不同 I/O 设备。

</details>


---

← — · [本章导读](../README.md) · [§10.2 →](./section-10.2-文件.md)
