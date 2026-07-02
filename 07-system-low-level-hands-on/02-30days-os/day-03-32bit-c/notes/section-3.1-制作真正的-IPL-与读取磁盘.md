## ① 制作真正的 IPL 与读取磁盘

Day 1–2 的 IPL **只显示文字**，**没有装载 OS 本体**，只是「假引导」。从 Day 3 起，IPL **名副其实**——调用 **BIOS 中断** 把软盘上的 **bootpack（OS 本体）** 读进内存，完成启动链装载。

| 小节 | 内容 |
|------|------|
| [§3.1.1 IPL、bootpack 与镜像布局](./section-3.1.1-IPL-bootpack与镜像布局.md) | 三个名字、启动链、**`0x00` 占位 ≠ bootpack** |
| [§3.1.2 软盘 CHS 结构与读盘范围](./section-3.1.2-软盘CHS结构与读盘范围.md) | 柱面 / 磁头 / 扇区、10 柱面 ≈ 180 KB |
| [§3.1.3 INT 0x13 与 ipl 代码拆解](./section-3.1.3-INT0x13与ipl代码拆解.md) | 读盘中断 API、重试 5 次、[ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm) |
| [§3.1.4 实模式读盘与保护模式切换](./section-3.1.4-实模式读盘与保护模式切换.md) | **16 位实模式** IPL vs **32 位保护模式** bootpack |

代码：[code/sec-3.1-ipl-int13-disk-load/](../code/sec-3.1-ipl-int13-disk-load/) · 构建：[code/sec-3.1-ipl-int13-disk-load/README.md](../code/sec-3.1-ipl-int13-disk-load/README.md)

---

### 本段带走什么

```text
BIOS 读 512B IPL → 0x7C00（16 位实模式）
       ↓ INT 0x13 循环读盘
bootpack → 内存 0x8200 起（约 180 KB）
       ↓ JMP 0x8200 → bootpack 切保护模式（见 §3.1.4 · §3.3）
```

---

### 自检

- [ ] 说清 **IPL / bootpack / `INT 0x13`** 各是什么（§3.1.1）
- [ ] 说清 **Day 2 的 `0x00` 占位** 与 **写入 bootpack** 的关系（§3.1.1）
- [ ] 会算 **10 柱面 × 2 头 × 18 扇区 × 512 B**（§3.1.2）
- [ ] 填对 **`INT 0x13` 读盘寄存器**（§3.1.3）
- [ ] 说清 **实模式 IPL** 与 **保护模式内核** 的分工，以及 **为何开机默认实模式**（§3.1.4）
- [ ] 能从 [ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm) **`nasm -f bin`** 得到 **512 B** 的 `ipl.bin`

---

← [Day 2 §2.3](../../day-02-asm-makefile/notes/section-2.3-先制作启动区.md) · [§3.1.1 →](./section-3.1.1-IPL-bootpack与镜像布局.md) · 下一节 [§3.2 纸娃娃操作系统](./section-3.2-纸娃娃操作系统.md)
