# TLPI 第 41 章 — Fundamentals of Shared Libraries

> 对应目录：`chapter-41-shared-libraries/`  
> （勿用 `…-shared-libraries-basics` — 与 [CHAPTER-MAP](../CHAPTER-MAP.md) 一致）  
> 书名原文：**Fundamentals of Shared Libraries**  
> ⚠️ **共享库要 `-fPIC`；运行时找的是 soname。** 新项目用 **RUNPATH**（可被 `LD_LIBRARY_PATH` 盖过）；SUID 忽略 `LD_LIBRARY_PATH`。默认全局符号可被主程序 **介入**。

**优先级**：🔴（部署、插件铺垫、ABI 版本）  
**前置**：[Ch40 登录记账](../chapter-40-login-accounting/notes.md)  
**后置**：[Ch42 共享库高级 / dlopen](../chapter-42-shared-libraries-advanced/notes.md)

---

## 章节目标

`.a` vs `.so`；PIC；soname 三名；构建安装/`ldconfig`；RPATH/RUNPATH；搜索顺序；符号介入与绑定；工具链。

---

## 41.1–41.3 静态 vs 共享

| | 静态 `.a` | 共享 `.so` |
|--|-----------|------------|
| 链接 | 代码拷进可执行文件 | 记 `DT_NEEDED` |
| 运行 | 无外部库依赖 | `ld-linux` 加载；多进程可共享映射 |
| 升级 | 须重链程序 | ABI 兼容可换库 |

---

## 41.4 构建

```bash
gcc -c -fPIC foo.c
gcc -shared -Wl,-soname,libfoo.so.1 -o libfoo.so.1.0.0 foo.o
ln -s libfoo.so.1.0.0 libfoo.so.1
ln -s libfoo.so.1 libfoo.so          # -lfoo
gcc -o app main.c -L. -lfoo -Wl,--enable-new-dtags,-rpath,'$ORIGIN'
```

| 名称 | 例 |
|------|-----|
| Real | `libfoo.so.1.0.0` 实体文件 |
| **Soname** | `libfoo.so.1`（写进 ELF；运行时找它） |
| Linker | `libfoo.so`（`-lfoo`） |

**PIC**：相对寻址，可映射任意基址；建 `.so` 必备 `-fPIC`。

Demo 树：[`code/`](./code/)

---

## 41.5 工具

`ldd ./app` · `readelf -d`（NEEDED/SONAME/RPATH/RUNPATH）· `objdump -T`  
⚠️ 对不可信二进制慎用 `ldd`（会跑动态链接器）；静态看依赖用 `readelf`。

---

## 41.6–41.9 版本与安装

主版本变（`.so.1`→`.so.2`）= ABI 不兼容。  
安装到库目录 + `ld.so.conf.d` + **`ldconfig`**（刷软链与 `/etc/ld.so.cache`）。

---

## 41.10–41.11 RPATH / RUNPATH · 搜索顺序

| | |
|--|--|
| **DT_RPATH**（旧） | 常 **高于** `LD_LIBRARY_PATH` |
| **DT_RUNPATH**（推荐，`--enable-new-dtags`） | **低于** `LD_LIBRARY_PATH` |

`$ORIGIN` = 可执行文件目录。

查找（库名无 `/` 时，示意）：

1. RPATH（若无 RUNPATH）  
2. `LD_LIBRARY_PATH`（**SUID/SGID 忽略**）  
3. RUNPATH  
4. `ld.so.cache`  
5. `/lib` `/usr/lib`（及 64 位变体）  

---

## 41.12 符号解析 · Interposition

默认全局符号扁平：主程序全局符号可覆盖库内同名符号（库内调用也可能进主程序）→ **符号介入**。  
缓解：库链 `-Bsymbolic`。  

绑定：默认 **lazy**（PLT 首次调用解析）；`LD_BIND_NOW=1` 立即绑定。

---

## 易错清单

1. 忘 `-fPIC`  
2. 搞混 real/soname/linker name  
3. 装库不 `ldconfig`  
4. SUID + 依赖 `LD_LIBRARY_PATH`  
5. RPATH vs RUNPATH  
6. 未预期的符号介入  

---

## 实验清单

1. 带 soname 的三链构建  
2. RUNPATH vs `LD_LIBRARY_PATH`  
3. （选）interposition / `-Bsymbolic`  
4. `LD_BIND_NOW`  
5. `$ORIGIN`  

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

## 参考

- Kerrisk · TLPI Ch41  
- `man 8 ldconfig` · `man 1 ldd` · `man 1 ld.so`
