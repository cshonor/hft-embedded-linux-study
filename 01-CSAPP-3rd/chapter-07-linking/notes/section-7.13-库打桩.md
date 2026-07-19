## 7.13 库打桩 (Interpositioning)

> **Ch7 §7.13** · [章导读](../README.md) · 上节 [§7.12 ←](./section-7.12-PIC位置无关代码.md) · 下节 [§7.14 →](./section-7.14-处理目标文件的工具.md)

---

在 **malloc/free、pthread、socket** 等库调用路径插入自己的实现 — 用于 **调试、统计、模拟**。

| 方式 | 机制 |
|------|------|
| **7.13.1 编译时** | `#define malloc my_malloc` |
| **7.13.2 链接时** | **静态库顺序**：`libwrap.a` 在 `-lc` 前，强符号覆盖 |
| **7.13.3 运行时** | **`LD_PRELOAD=libwrap.so`** — 动态符号 interpose |

```bash
LD_PRELOAD=./libmwrap.so ./prog
```

**HFT：** 开发/压测用 **malloc 统计、延迟 trace**；**生产禁用** 未审计的 `LD_PRELOAD`（安全风险）。

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

---

← [§7.12 ←](./section-7.12-PIC位置无关代码.md) · [本章导读](../README.md) · [§7.14 →](./section-7.14-处理目标文件的工具.md)
