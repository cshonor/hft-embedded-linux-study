## 10.8 共享文件

> **Ch10 §10.8** · [章导读](../README.md) · 上节 [§10.7 ←](./section-10.7-读取目录内容.md) · 下节 [§10.9 →](./section-10.9-I-O重定向.md)

---

三层结构：

```
进程 fd 表 → 打开文件表 → v-node 表（inode 内容）
```

| 机制 | 效果 |
|------|------|
| 两 fd 同 **打开文件表项** | 共享 **文件偏移** |
| `fork` | 父子共享已打开文件的偏移 |
| 不同 `open` 同一路径 | 通常 **独立偏移** |

- **`O_APPEND`** — 写前内核把偏移设到末尾，原子追加

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. 描述符表、文件表、v-node 表三级结构是什么？fork 后 fd 怎么共享？</summary>

**三级结构**：
1. **描述符表**(per-process)：fd → 文件表项指针
2. **文件表**(kernel-global)：文件偏移(offset) + 状态flags + v-node 指针
3. **v-node 表**(kernel-global)：inode 信息(文件大小、类型等)

**fork 后**：子进程复制描述符表 → 指向**同一个文件表项** → 共享 offset。所以父子进程写同一 fd 会互相追加。**dup 后**：同一进程的两个 fd 指向同一文件表项 → 也共享 offset。

</details>

<details>
<summary>2. 两个进程分别 open 同一文件会共享 offset 吗？</summary>

**不共享**。每个 `open` 创建新的文件表项（独立 offset）。只有 `fork`（复制描述符表）和 `dup`（复制 fd）才共享文件表项（共享 offset）。这就是 shell 重定向 `> file 2>&1` 的原理——`dup2(fd, STDOUT_FILENO)` 让 stdout 和 fd 指向同一文件表项，共享 offset。

</details>


---

← [§10.7 ←](./section-10.7-读取目录内容.md) · [本章导读](../README.md) · [§10.9 →](./section-10.9-I-O重定向.md)
