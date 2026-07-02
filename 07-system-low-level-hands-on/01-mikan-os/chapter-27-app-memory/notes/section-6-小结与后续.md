## 6. 小结与后续

---

### 一、本章总结

| 成果 | 说明 |
|------|------|
| **Demand Paging** | **#PF 分配** · **DemandPages/sbrk** |
| **MapFile** | **按页 fault-in 文件** |
| **memstat** | **物理帧统计** |
| **CoW + invlpg** | **多 cube 省 RAM** |

**统一主题：** **Page Fault = OS 内存策略入口** — 而非仅 **KillApp**。

---

### 二、与前期章节链

```
Ch8  位图分配
Ch19 SetupPageMaps（eager）
Ch24 每应用 PML4
Ch25 sbrk 简化
Ch27 Demand · MapFile · CoW  ← 本章
```

---

### 三、HFT 阅读建议

| 优先级 | 内容 |
|--------|------|
| **🔴 必读** | **Demand Paging · #PF handler · CoW 概念** |
| **🟡 选读** | **MapFile 细节** — 对照 **mmap** |
| **⚪ 可跳过** | **memstat 命令 UI** — 知道用途即可 |

---

### 四、后续索引

| Ch27 主题 | 继续读 |
|----------|--------|
| 日文/重定向 | [chapter-28-japanese-redirect](../chapter-28-japanese-redirect/) ⚪ |
| IPC | [chapter-29-ipc](../chapter-29-ipc/) |
| 分页基础 | [chapter-19-paging](../chapter-19-paging/) 🔴 |
| CSAPP VM | [01-CSAPP-3rd Ch9](../../../01-CSAPP-3rd/chapter-09-virtual-memory/) |

---

← [5. CoW](./section-5-写入时复制与invlpg.md) · [Ch 26](../chapter-26-app-write-file/) · [Ch 27 导读](../README.md)
