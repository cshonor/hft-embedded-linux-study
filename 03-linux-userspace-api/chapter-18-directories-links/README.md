# TLPI 第 18 章 — Directories and Links

**优先级**：🔴（路径树操作、临时文件、TOCTOU、可靠写入）  
**前置**：[Ch17 ACL](../chapter-17-access-control-lists/notes.md) · [Ch14 FS/inode](../chapter-14-file-systems/notes.md) · [Ch15 stat/lstat](../chapter-15-file-attributes/notes.md)  
**后置**：[Ch19 inotify](../chapter-19-monitoring-file-events/notes.md)

---

## 小节目录

- [18.1 目录基础](./notes/18.1-directory.md)
- [18.2 读目录](./notes/18.2-directory.md)
- [18.3 硬链接](./notes/18.3-link.md)
- [18.4 符号链接](./notes/18.4-link-symbol.md)
- [18.5 `rename`（同 FS 原子）](./notes/18.5-rename.md)
- [18.6 路径与 TOCTOU](./notes/18.6-toctou.md)

---

## 章节目标


理清目录 / 硬链 / 软链；掌握 mkdir/rmdir、目录遍历、link/symlink/unlink、`rename` 原子性；用 `*at` + flags 缓解路径竞态。

---


---

## 18.7 易错清单


1. 目录要访问内容：常需 `x`，不只 `r`  
2. 硬链禁目录；软链可以  
3. `unlink` 减计数，不是立刻抹数据  
4. `rename` 原子性限于**同一文件系统**  
5. `readlink` ≠ 解析最终路径  
6. 硬链不新建数据 inode；软链新建 inode  
7. 无「递归 rmdir」syscall；自行遍历 unlink + rmdir  
8. 根 `..` 指自己，遍历防环  

---


---

## 练习


1. `opendir`/`readdir` + `lstat` 区分类型  
2. `link` vs `symlink`，比 `st_ino`  
3. 临时文件 + `rename` 安全写  
4. open 后 unlink 仍可读  
5. 目录仅 `r` 无 `x` 的行为  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 目录项存名字→inode；权限 r/x/w 语义不同 |
| 2 | 硬链共享 inode，同 FS，不链目录 |
| 3 | 软链独立 inode，可跨 FS / 悬空 |
| 4 | `nlink==0` 且无 fd → 才释放 |
| 5 | `rename` 同 FS 原子；可靠写 = write tmp + rename |
| 6 | 用 `*at` + `AT_SYMLINK_NOFOLLOW` 减 TOCTOU |

---


---

## 参考


- Kerrisk · TLPI Ch18  
- `man 2 mkdir` · `man 3 opendir` · `man 2 link` · `man 2 symlink` · `man 2 rename` · `man 2 unlink`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
