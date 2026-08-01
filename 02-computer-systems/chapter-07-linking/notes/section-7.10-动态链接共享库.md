## 7.10 动态链接共享库

> **Ch7 §7.10** · [章导读](../README.md) · 上节 [§7.9 ←](./section-7.9-加载可执行目标文件.md) · 下节 [§7.11 →](./section-7.11-从应用程序加载共享库.md)

---

- **`.so`** — 运行时 **共享** 一份物理代码（TEXT 共享、DATA 每进程副本）
- **节省磁盘与 RAM**；**升级 libc** 不必重编所有程序
- 链接器 `ld.so` / `ld-linux-x86-64.so.2` **重定位共享库** 并绑定符号

**延迟绑定 (lazy binding)：**  
`PLT` (Procedure Linkage Table) + `GOT` (Global Offset Table) — 首次调用跳 `ld.so` 解析，之后走缓存地址。

```bash
ldd ./prog
LD_DEBUG=bindings ./prog   # 调试加载
```

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§7.9 ←](./section-7.9-加载可执行目标文件.md) · [本章导读](../README.md) · [§7.11 →](./section-7.11-从应用程序加载共享库.md)
