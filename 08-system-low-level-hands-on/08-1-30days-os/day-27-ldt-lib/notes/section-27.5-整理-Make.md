## ⑤ 整理 Make · 目录重构

#### 目录

| 目录 | 内容 |
|------|------|
| **`haribote/`** | OS 内核 |
| **`apilib/`** | API 库源码 → **`apilib.lib`** |
| **各 app 子目录** | `hello3/`、`color2/` … |

#### `app_make.txt`

**`include` 共通规则** — 每个 app 的 **Makefile 仅数行**。

#### 全局目标

| 命令（示意） | 用途 |
|--------------|------|
| **`make run_full`** | 编全盘 + 跑 |
| **`make install_full`** | 写入软盘映像 |

**从手工作坊 → 可维护工程** — HFT 里 **monorepo + 共享 static lib + 顶层 CMake** 同构。

→ [Day 2 Makefile](../day-02-asm-makefile/) · [Day 6 模块拆分](../day-06-split-compile-irq/)

---
