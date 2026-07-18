## 3.2.1 机器级代码：完整编译链路

> [章导读](../README.md) · 上节 [§3.1](./section-3.1-历史观点.md) · 下节 [§3.2.2 栈帧](./section-3.2.2-栈帧.md)

---

```
C 源码 (.c)
    ↓  编译器（gcc/clang）
汇编 (.s)          ← gcc -S 停在这里，人类可读
    ↓  汇编器 (as)
目标文件 (.o)      ← 机器码 + 重定位信息，尚不可直接运行
    ↓  链接器 (ld)
可执行程序         ← Linux ELF · UEFI 则为 PE（BOOTX64.EFI）
```

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

1. **学汇编用 `-O0` 还是 `-Og`/`-O3`？** — **学习用 `-Og`**；**压测/上线看 `-O3`**  
2. **`.efi` 和本章编译链关系？** — 同样 **C → .o → 链接**；UEFI 链出 **PE** 而非 ELF

---

← [本章导读](../README.md) · [§3.1 ←](./section-3.1-历史观点.md) · [§3.2.2 →](./section-3.2.2-栈帧.md)
