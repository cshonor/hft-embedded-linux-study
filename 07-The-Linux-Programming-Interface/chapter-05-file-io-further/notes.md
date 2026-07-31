# TLPI 第 05 章 — File I/O: Further Details

> 目录：`chapter-05-file-io-further/`（与书内第 5 章对齐）  
> 通用 I/O 模型在 **第 4 章** → [`../chapter-04-file-io-universal/`](../chapter-04-file-io-universal/)

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

前置通读：[书内 Ch4 通用 I/O](../chapter-04-file-io-universal/notes.md)

---

## 参考

- 《The Linux Programming Interface》**第 05 章** — File I/O: Further Details  
- [OUTLINE](../OUTLINE.md)
