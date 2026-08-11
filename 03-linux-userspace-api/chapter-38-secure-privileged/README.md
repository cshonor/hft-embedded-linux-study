# TLPI 第 38 章 — Writing Secure Privileged Programs

**优先级**：🔴（SUID、daemon 降权、攻击面）  
**前置**：[Ch9 凭证](../chapter-09-process-credentials/notes.md) · [Ch37 Daemons](../chapter-37-daemons/notes.md)  
**后置**：[Ch39 Capabilities](../chapter-39-capabilities/notes.md)

---

## 小节目录

- [38.1 两类特权程序](./notes/38.1-section-38-1.md)
- [38.2 丢弃与恢复（核心）](./notes/38.2-section-38-2.md)
- [38.3 安全准则（精要）](./notes/38.3-security.md)

---

## 章节目标


SUID/SGID 安全模型；`setuid` vs `seteuid`；临时/永久丢权；TOCTOU、环境、符号链接、shell 注入；多层准则；衔 Capability。

---


---

## 易错清单


1. root 下用 `setuid`「临时」降权 → 回不去  
2. `system` + 脏 PATH  
3. `stat`→`open` TOCTOU  
4. 全程持 root 跑复杂逻辑  
5. fork 继承 UID；SUID 行为在 exec 时生效  

---


---

## 实验清单


1–2. Ch9 临时/永久降权  
3. `open`+`fstat` vs TOCTOU  
4. （选）PATH 劫持对比  
5. `O_NOFOLLOW`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 权限看 EUID；Saved 管恢复 |
| 2 | seteuid 临时；setuid(root) 永久 |
| 3 | 少 SUID → Capability / daemon |
| 4 | 禁 system；绝对路径 exec |
| 5 | open+fstat，防 TOCTOU |
| 6 | 清环境、校验输入、关多余 fd |

---


---

## 参考


- Kerrisk · TLPI Ch38  
- [Ch9 notes](../chapter-09-process-credentials/notes.md) · `man 7 credentials` · `man 2 seteuid`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
