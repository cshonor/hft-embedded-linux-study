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

### 自测题

<details>
<summary>1. 什么是库打桩(library interpositioning)？有哪几种方式？</summary>

库打桩 = 拦截对库函数的调用，插入自定义代码。三种方式：
1. **编译时**：`-Dmalloc=mymalloc` 宏替换
2. **链接时**：`--wrap malloc` 让链接器把 `malloc` 调用改为 `__wrap_malloc`
3. **运行时**：`LD_PRELOAD=./mymalloc.so` 让动态链接器先加载你的库

用途：内存泄漏检测、性能 profiling、mock 测试。HFT：用 `LD_PRELOAD` 注入延迟模拟器测试系统在恶劣网络下的表现。

</details>


---

← [§7.12 ←](./section-7.12-PIC位置无关代码.md) · [本章导读](../README.md) · [§7.14 →](./section-7.14-处理目标文件的工具.md)
