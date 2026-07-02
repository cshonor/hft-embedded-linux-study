## 2. make 与 Makefile

> 此前每次改内核都要手动敲 **`clang++`**、**`ld.lld`** — 易错且不可重复。

---

### 一、make 做什么

**make** — 根据 **Makefile** 描述，自动判断 **哪些目标过期** 并执行对应命令。

```
源文件变更 → make 只重编受影响目标
无变更     → 跳过（节省时间）
```

| 概念 | 含义 |
|------|------|
| **目标（target）** | 要生成的文件，如 `kernel.elf` |
| **依赖（prerequisites）** | 目标由哪些文件构成 |
| **命令（recipe）** | 生成目标的 shell 命令（**必须以 Tab 缩进**） |

---

### 二、Makefile 最小示例（结构）

```makefile
kernel.elf: main.o pixel_writer.o
	clang++ -o kernel.elf main.o pixel_writer.o -nostdlib ...

main.o: main.cpp
	clang++ -c main.cpp -o main.o
```

| 规则 | 说明 |
|------|------|
| **`kernel.elf: a.o b.o`** | elf 依赖两个 .o |
| **下一行 Tab + 命令** | 链接生成 elf |
| **模式可扩展** | `CXX`、`LDFLAGS` 变量 — 书中逐步完善 |

→ 对照 [01 Day 3 Makefile](../../02-30days-os/day-03-makefile/)（若已读 30 天 OS）

---

### 三、MikanOS 工程收益

| 之前 | 之后 |
|------|------|
| Loader / Kernel 各敲一长串命令 | **`make`** 一键构建 |
| 漏链某个 .o | 依赖写进 Makefile — **可复现** |
| 与 EDK II 构建并存 | 内核侧 **独立 Makefile** — 职责清晰 |

**习惯：** 改代码 → `make` → QEMU — 与后续章节增量开发一致。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. WritePixel](./section-3-像素格式与WritePixel.md)
