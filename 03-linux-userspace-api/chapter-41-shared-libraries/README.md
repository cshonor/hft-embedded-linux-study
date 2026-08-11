# TLPI 第 41 章 — Fundamentals of Shared Libraries

**优先级**：🔴（部署、插件铺垫、ABI 版本）  
**前置**：[Ch40 登录记账](../chapter-40-login-accounting/notes.md)  
**后置**：[Ch42 共享库高级 / dlopen](../chapter-42-shared-libraries-advanced/notes.md)

---

## 小节目录

- [41.1 –41.3 静态 vs 共享](./notes/41.1-static.md)
- [41.4 构建](./notes/41.4-section-41-4.md)
- [41.5 工具](./notes/41.5-tools.md)
- [41.6 –41.9 版本与安装](./notes/41.6-section-41-6.md)
- [41.10 –41.11 RPATH / RUNPATH · 搜索顺序](./notes/41.10-rpath-runpath.md)
- [41.12 符号解析 · Interposition](./notes/41.12-interposition.md)

---

## 章节目标


`.a` vs `.so`；PIC；soname 三名；构建安装/`ldconfig`；RPATH/RUNPATH；搜索顺序；符号介入与绑定；工具链。

---


---

## 易错清单


1. 忘 `-fPIC`  
2. 搞混 real/soname/linker name  
3. 装库不 `ldconfig`  
4. SUID + 依赖 `LD_LIBRARY_PATH`  
5. RPATH vs RUNPATH  
6. 未预期的符号介入  

---


---

## 实验清单


1. 带 soname 的三链构建  
2. RUNPATH vs `LD_LIBRARY_PATH`  
3. （选）interposition / `-Bsymbolic`  
4. `LD_BIND_NOW`  
5. `$ORIGIN`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | `.so` 要 PIC；运行找 soname |
| 2 | real / soname / linker 三名 |
| 3 | 新项目用 RUNPATH + `$ORIGIN` |
| 4 | 搜索序：RPATH→LDLP→RUNPATH→cache→默认 |
| 5 | SUID 忽略 LD_LIBRARY_PATH |
| 6 | 默认可符号介入；lazy PLT |

---


---

## 参考


- Kerrisk · TLPI Ch41  
- `man 8 ldconfig` · `man 1 ldd` · `man 1 ld.so`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
