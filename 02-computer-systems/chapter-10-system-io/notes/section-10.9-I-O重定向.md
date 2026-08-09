## 10.9 I/O 重定向

> **Ch10 §10.9** · [章导读](../README.md) · 上节 [§10.8 ←](./section-10.8-共享文件.md) · 下节 [§10.10 →](./section-10.10-标准I-O.md)

---

← [本章导读](../README.md)

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. `dup2(oldfd, newfd)` 做了什么？shell 中 `>` 重定向如何实现？</summary>

`dup2` 把 `oldfd` 复制到 `newfd`——先关闭 `newfd`（如果打开），再让 `newfd` 指向 `oldfd` 的文件表项。两者共享同一 offset。

shell `>` 重定向实现：`fork` → 子进程中 `close(STDOUT_FILENO)` → `open("file", O_WRONLY)`（获得最小 fd = 1 = stdout）→ `exec`。或用 `dup2(fd, STDOUT_FILENO)` 更简洁。`2>&1` = `dup2(STDOUT_FILENO, STDERR_FILENO)`。

</details>


---

← [§10.8 ←](./section-10.8-共享文件.md) · [本章导读](../README.md) · [§10.10 →](./section-10.10-标准I-O.md)
