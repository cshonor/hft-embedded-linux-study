# 11.6 KDB：内核内置调试器

> 🔴 精读

## 本节要点

### KDB 基本用法

```bash
# 进入 KDB (与 KGDB 相同的入口)
echo g > /proc/sysrq-trigger

# KDB 提示符
kdb>

# 常用命令
kdb> help              # 帮助
kdb> bt                # 当前栈回溯
kdb> btp <pid>         # 指定进程栈回溯
kdb> ps                # 进程列表
kdb> go                # 继续运行
kdb> ss                # 单步
kdb> bp <addr>         # 设置断点
kdb> bc <num>          # 清除断点
kdb> md <addr>         # 查看内存 (dump)
kdb> rd                # 查看寄存器
kdb> dmesg             # 查看内核日志
```

### KDB vs KGDB

| 特性 | KDB | KGDB |
|------|-----|------|
| 需要 GDB | ❌ | ✅ |
| 源码级调试 | ❌ | ✅ |
| 查看变量 | 有限 | ✅ 完整 |
| 操作便利 | 直接在控制台 | 需要开发机 |
| 适用 | 快速排查 | 深度调试 |

### 切换

```bash
# KGDB 模式下按 Ctrl+C 切到 KDB
# KDB 模式下输入 kgdb 切到 KGDB
kdb> kgdb
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KDB 和 KGDB 如何切换？

> KGDB 模式（GDB 连接中）按 Ctrl+C 中断 GDB 连接，自动切到 KDB。KDB 模式输入 `kgdb` 命令切回 KGDB，等待 GDB 重新连接。切换不需要重启内核。

</details>
