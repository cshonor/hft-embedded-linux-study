## ⑤ 神奇的 SysRq 键 · Magic SysRq

**`CONFIG_MAGIC_SYSRQ`** — 系统 **死锁/假死** 时仍可向内核发 **底层命令**。

| x86 组合 | **`Alt + SysRq(PrtSc) + 字母`** |

| 命令 | 含义 |
|------|------|
| **`SysRq-s`** | **sync** — 脏缓冲写盘 |
| **`SysRq-u`** | **umount** — 卸载 FS |
| **`SysRq-b`** | **reboot** — 立即重启（**未 sync 会丢数据**） |

| 救命序列（经典） | **`s` → `u` → `b`** — 尽量安全重启 |

```
系统无响应但 SysRq 仍通
    ▼
s（落盘）→ u（卸盘）→ b（重启）
```

**HFT 实盘：** 慎用；Prefer **优雅下线** — 懂即可。

---
