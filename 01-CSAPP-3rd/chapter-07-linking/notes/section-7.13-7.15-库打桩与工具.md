## 7.13–7.15 库打桩、工具与小结

### 7.13 库打桩 (Interpositioning)

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

### 7.14 处理目标文件的工具

| 工具 | 用途 |
|------|------|
| `ar` | 创建/查看静态库 |
| `nm` | 符号表 |
| `objdump -d/-r/-t` | 反汇编、重定位、符号 |
| `readelf -a` | ELF 头、节、段、动态段 |
| `size` | 各段大小 |
| `strings` | 可打印串 |
| `ldd` | 动态依赖 |
| `strip` | 去符号减体积 |

### 7.15 小结（原书）

- 链接 = **符号解析 + 重定位 + 库合并**
- 静态 vs 动态 = **部署、启动、升级** 权衡
- 与 **加载、VM** 一体 — 下一 Part II 章继续系统行为

→ [Ch 8 异常控制流](../chapter-08-exceptional-control-flow/) · [Ch 9 VM](../chapter-09-virtual-memory/)

---

← [本章导读](../README.md)
