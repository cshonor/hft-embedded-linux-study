## ③ 32 位模式前期准备与导入 C 语言

**本章核心转折。** 实模式铺垫见 [§3.1.4](./section-3.1.4-实模式读盘与保护模式切换.md)；汇编与 C 分工见 [§3.4](./section-3.4-汇编与-C-的结合.md)。

| 小节 | 内容 |
|------|------|
| [§3.3.1 16/32/64 分工与 Load vs Run](./section-3.3.1-16-32-64分工与Load-vs-Run.md) | 三档 CPU 模式、**不是再从磁盘装内核** |
| [§3.3.2 进 32 位前 BIOS 与启动链](./section-3.3.2-进32位前BIOS与启动链.md) | 为何先 16 位做完 BIOS、Day 3 四阶段 |
| [§3.3.3 32 位保护模式与段/页](./section-3.3.3-32位保护模式与段页.md) | A20/GDT/CR0、**段 vs 页** |
| [§3.3.4 16→32→64 阶梯](./section-3.3.4-16-32-64阶梯.md) | 没法跳级、64 依赖 32、你现在卡在哪 |
| [§3.3.5 MikanOS 与 UEFI 对照](./section-3.3.5-MikanOS与UEFI对照.md) | 02 川合 vs 02 Mikan、UEFI 怎么理解 |
| [§3.3.6 引入 C 与嵌入式/HFT](./section-3.3.6-引入C与嵌入式HFT.md) | **`bootpack.c` / `HariMain`**、要不要写 asm |

---

### 本段带走什么

```text
16 位：Load + BIOS（IPL 读 bootpack；nasmhead 切 VGA）
       ↓
32 位：Run（nasmhead 切模式 → HariMain 跑 C）
       ↓
64 位：原书不做；MikanOS / Linux 另论
```

---

### 自检

- [ ] 说清 **16 Load vs 32 Run**（§3.3.1）
- [ ] 说清 **进 32 位前必须做完的 BIOS 事**（§3.3.2）
- [ ] 区分 **段 vs 页**（§3.3.3）
- [ ] 说清 **16→32→64 阶梯** 与 **64 依赖 32 什么**（§3.3.4）
- [ ] 对照 **haribote vs MikanOS / UEFI**（§3.3.5）
- [ ] 找到 **`HariMain`** 与 [§3.4 asm+C](./section-3.4-汇编与-C-的结合.md) 的关系（§3.3.6）

---

← [§3.2 纸娃娃 OS](./section-3.2-纸娃娃操作系统.md) · [§3.3.1 →](./section-3.3.1-16-32-64分工与Load-vs-Run.md) · [§3.4 汇编与 C →](./section-3.4-汇编与-C-的结合.md)
