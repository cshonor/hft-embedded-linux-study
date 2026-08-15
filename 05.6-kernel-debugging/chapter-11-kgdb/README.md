# Ch11 Using Kernel GDB (KGDB)

> Part 3: Diagnostics & Advanced Tools · 🔴 精读

KGDB 源码级调试：串口配置、断点 / 单步 / 查看变量、调试内核模块、树莓派 UART 配置、KGDB + KDB (内核调试器) 组合使用。

---

## 小节索引

| 小节 | 笔记文件 |
|------|----------|
| 11.1 KGDB 原理与架构 | `notes/01-kgdb-architecture.md` |
| 11.2 串口配置（含树莓派 UART） | `notes/02-uart-setup.md` |
| 11.3 GDB 连接内核 | `notes/03-gdb-connection.md` |
| 11.4 断点 / 单步 / 查看变量 | `notes/04-breakpoints-variables.md` |
| 11.5 调试内核模块 (loadable module) | `notes/05-module-debugging.md` |
| 11.6 KDB：内核内置调试器 | `notes/06-kdb-builtin-debugger.md` |
| 11.7 KGDB 与 QEMU 虚拟机调试 | `notes/07-qemu-kgdb.md` |

---

## HFT 关联

精读。写内核模块时最高效的调试方式。树莓派 5 需通过 GPIO 14/15 (UART) 连接串口调试线。
