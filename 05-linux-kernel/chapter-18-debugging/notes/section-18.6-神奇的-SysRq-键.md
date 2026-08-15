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

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 系统完全死锁（键盘无响应）时，SysRq 还能用吗？

<details><summary>答案</summary>

能。SysRq 是键盘控制器硬件中断，直接触发 CPU NMI（不可屏蔽中断），绕过所有软件层（包括调度器/锁）。即使内核死锁在自旋锁上，NMI 中断处理仍能执行。这就是为什么 SysRq 被称为"神奇的"——它是最后的救命稻草。HFT 系统建议配置 kdump，在 SysRq+c 触发 panic 时自动保存 vmcore。

</details>

**Q2.** 为什么 SysRq+s → u → b 的顺序很重要？

<details><summary>答案</summary>

s(sync) 把脏页写回磁盘 → u(umount) 卸载文件系统标记干净 → b(reboot) 立即重启。如果跳过 s 直接 b，脏页丢失（文件系统损坏）。如果跳过 u 直接 b，文件系统标记为"未干净卸载"，重启后 fsck 检查。s→u→b 是最小化数据丢失的安全重启序列。生产环境也可以用 `echo s > /proc/sysrq-trigger` 通过命令行触发。

</details>

</details>

---
