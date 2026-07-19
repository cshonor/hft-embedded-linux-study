## 3.2.1 机器级代码：完整编译链路

> [章导读](../README.md) · 上节 [§3.1](./section-3.1-历史观点.md) · 下节 [§3.2.2 栈帧](./section-3.2.2-栈帧.md)

> **核心结论（先钉死）：** `as`（汇编器）、`ld`（链接器）**不属于 GCC**，它们是 **GNU Binutils** 里的独立工具。  
> `gcc` 是 **驱动包装程序**：内部自动调度 `cpp` / `cc1` / `as` / `ld` 跑完整条链。

---

### `gcc` / `as` / `ld` 各是谁？

| 名字 | 归属 | 干什么 |
|------|------|--------|
| **gcc** | GNU Compiler Collection（驱动 + `cc1` 等） | 调度整条流水线；**C→汇编** 主要靠自带的 **`cc1`**（高级语言前端） |
| **as** | **Binutils**（独立） | `.s` → `.o`（汇编成机器码目标文件） |
| **ld** | **Binutils**（独立） | 多个 `.o` + 库 → 可执行文件（解析符号、重定位） |

**误区：** ❌「`as`/`ld` 是 gcc 内置组件」→ ✅「gcc 只是调度器；`as`/`ld` 是系统里单独安装的二进制」。

同属 Binutils、HFT/CSAPP 常用：`objdump`（看机器码）、`readelf`、`nm`。

---

### 四步拆解（`test.c`）

```
C 源码 (.c)
    ↓  ① 预处理 cpp（展开头文件/宏）     →  .i
    ↓  ② 编译 cc1（C → 汇编文本）        →  .s     ← gcc -S 停这
    ↓  ③ 汇编 as（Binutils）              →  .o     ← 机器码 + 重定位，还不能直接跑
    ↓  ④ 链接 ld（Binutils）              →  可执行  ← Linux ELF · UEFI 则为 PE
```

```bash
gcc -E test.c -o test.i          # ① 预处理
gcc -S test.i -o test.s          # ② C→汇编（实际调 cc1）
as test.s -o test.o              # ③ 也可：gcc -c test.s -o test.o（gcc 帮你调 as）
ld …                             # ④ 裸 ld 要手写一堆启动/库参数；日常用 gcc 驱动更省事
gcc test.c -o test               # 一条命令：自动串联 cpp → cc1 → as → ld
```

手写纯汇编时可 **完全不经过 gcc**：

```bash
as test.s -o test.o
ld test.o -o test                # 极简 syscall 示例才这么裸；带 libc 仍建议 gcc 当驱动链接
./test
```

→ 驱动与链接细节：[Ch7 §7.1](../../chapter-07-linking/notes/section-7.1-编译器驱动程序.md)

**优化级别 — 两条线各取所需：**

| 选项 | 用途 | 说明 |
|------|------|------|
| **`-Og`** | **学汇编、对照 CSAPP** | 代码清晰、结构可读 — **读 `-S` 输出必用** |
| **`-O3`** | **HFT 上线、性能压测** | 激进优化 — **只看 Release 热路径汇编** |
| **`objdump -d`** | 调试内核 / 交易程序 | 对 **已有二进制** 反汇编，不依赖源码 |

```bash
gcc -Og -S -fno-verbose-asm hello.c -o hello.s   # 学习：看 C → 汇编（默认 AT&T）
gcc -O3 -S hotpath.c -o hotpath-O3.s             # HFT：看优化后真实形态
objdump -d ./a.out | less                        # Linux 默认 AT&T；HFT 共置机常用
# 不要用 -M intel —— 本路径只练 AT&T，与 CSAPP / perf annotate 一致
```

**MikanOS Ch1 补充：** UEFI 的 `.efi` 同样是 **C → 交叉编译 → .o → 链接成 PE**，只是工具链用 `clang -target x86_64-pc-win32-coff` + `lld-link`，不是宿主机 `gcc` 默认的 ELF 路径 → [MikanOS §2 交叉编译](../../../08-system-low-level-hands-on/01-mikan-os/chapter-01-hello-world/notes/section-2-二进制编辑器与BOOTX64.md)。

### 学习顺序建议（先 MikanOS 实操，再啃 CSAPP）

```
1. MikanOS Ch1：写 UEFI C、make 出 bootX64.efi — 先跑通
2. 对 hello.c 做 gcc/clang -S、objdump -d — 对照 §3.2.1–3.2.3
3. 遇汇编/寄存器/栈看不懂 → 回读 §3.2.2、§3.7
4. 内核基础跑通后 → 深入 Ch4 流水线，服务 HFT 延迟优化
```

**不必死磕顺序：** CSAPP Ch3 可与 MikanOS **交叉读**。

---

### 口述巩固 · 自测

1. **`as`/`ld` 属于 GCC 吗？** — **否**；属 **Binutils**；`gcc` 是驱动  
2. **四步各产出什么？** — `.i` → `.s`（cc1）→ `.o`（as）→ 可执行（ld）  
3. **学汇编用 `-Og` 还是 `-O3`？** — **学习 `-Og`**；**压测/上线看 `-O3`**  
4. **`.efi` 和本章编译链关系？** — 同样 **C → .o → 链接**；UEFI 链出 **PE** 而非 ELF  

---

← [本章导读](../README.md) · [§3.1 ←](./section-3.1-历史观点.md) · [§3.2.2 →](./section-3.2.2-栈帧.md)
