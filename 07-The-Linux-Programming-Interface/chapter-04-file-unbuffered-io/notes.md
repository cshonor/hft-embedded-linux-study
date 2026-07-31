# TLPI 第 05 章 — File I/O: Further Details

> **勘误：** 本书第 **5** 章是 Further Details（`dup`/`fcntl`/原子追加等）。  
> 仓库目录 `chapter-04-file-unbuffered-io/` 对应 **书内第 5 章**（目录编号历史错位）。  
> 通用 I/O 模型（`open/read/write/close`）在 **书内第 4 章** → [`../chapter-03-file-io/`](../chapter-03-file-io/)

## 学习状态

- [ ] 已通读
- [ ] 已做笔记
- [ ] C 示例已跑
- [ ] Rust 对照已写

**优先级**：🟡（HFT：非阻塞、`pread`/`pwrite`、fd 复制与偏移共享）

---

## 占位 · 待展开

- fd 复制：`dup` / `dup2` / `fcntl(F_DUPFD)`
- `pread` / `pwrite`（不改共享 offset）
- 原子追加 `O_APPEND`
- `fcntl`、非阻塞 I/O
- 大文件 / `off_t`
- 内核三层：fd 表 ↔ open file description ↔ inode

前置通读：[书内 Ch4 通用 I/O](../chapter-03-file-io/notes.md)

---

## 参考

- 《The Linux Programming Interface》**第 05 章** — File I/O: Further Details  
- [OUTLINE](../OUTLINE.md)
