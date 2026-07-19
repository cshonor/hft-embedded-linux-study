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

### 优化等级 `-O`（大写字母 O，不是数字 0）

`-O` = **Optimize**。数字越大通常越激进。写错成 `OG3` 之类无效；正确是 **`-O3`**（O + 3）。

| 等级 | 定位 | 要点 |
|------|------|------|
| **`-O0`** | 默认 / 纯调试 | 几乎不优化；变量多在栈；顺序贴近源码；**gdb 友好**；**极慢 — 绝不当 HFT Release** |
| **`-O1`** | 轻度 | 常量折叠、删死代码、简单寄存器分配；编译快 |
| **`-O2`** | 生产通用 | 含 O1 + 更完整寄存器调度、指令调度、小函数自动 inline、公共子表达式等；**速度与体积较平衡**；多数后台服务常用 |
| **`-O3`** | **激进 / HFT 热路径常用** | 含 O2，再加更强 inline、循环展开、**向量化**（常开 `-ftree-vectorize`）、更多 IPA 等 — 少 `call`、少内存往返、贴流水线 |
| **`-Og`** | **学汇编 / 对照 CSAPP** | 介于可调试与一点速度之间；**读 `-S` 优先用这个**（比 O0 干净，比 O3 可读） |
| **`-Ofast`** | 更激进 | 可放宽 IEEE 浮点等；**HFT 金额/浮点路径慎用**（精度风险） |

**`-O3` 对低延迟常多出来的味道（相对 O2，随版本/目标略有差异）：**

1. **更强函数内联** — 中等函数也常展开，少 `call`/`ret`（对照 [§3.7.7 `inline`](./section-3.7-过程与栈帧.md)）  
2. **循环展开 / 向量化** — 可能生成 AVX/SIMD，批量算行情字段  
3. **更多跨函数 IPA** — 少多余拷贝、少栈倒腾  
4. **调度/分支相关优化** — 服务流水线（→ [Ch4](../../chapter-04-processor-architecture/README.md)）

| | `-O3` |
|--|-------|
| ✅ | 热路径周期更少；更接近「工业低延迟」真实汇编 |
| ❌ | 编译更慢、二进制更大；gdb 难跟；过度内联/展开可能伤 **icache**（负优化） |

**和 Ch3 的关系：** 同一份 C，`-O0`/`-Og` 与 `-O3` 生成的汇编可以差很多 — 学习看前者，**上线看后者**。

```bash
gcc -O0 test.c -o test_debug          # 调试 / 对照「臃肿」基础汇编
gcc -Og -S -fno-verbose-asm hello.c -o hello.s   # 学习：看 C → 汇编（默认 AT&T）
gcc -O3 -S hotpath.c -o hotpath-O3.s             # HFT：看优化后真实形态
gcc -O3 test.c -o test_hft            # 上线热路径常用（常再加 -march=native 等）
objdump -d ./test_debug ./test_hft | less        # 对比删了多少冗余、有没有 call
# 不要用 -M intel —— 本路径只练 AT&T，与 CSAPP / perf annotate 一致
```

#### 为何普通业务多用 `-O2`（甚至调试用 `-O0`），而 HFT 热路径用 `-O3`？

| 代价 / 差异 | 普通 Web·后台·管理系统 | HFT |
|-------------|------------------------|-----|
| **编译时间** | O3 全局 IPA/展开/强内联 → 大仓可慢 **数倍**；频繁改代码不可接受 → 生产常 **`-O2`**，日常调试 **`-O0`/`-Og`** | 热路径体量可控；可接受较长编译换周期 |
| **可调试性** | 线上要抓栈、gdb；O3 打乱次序、吃掉局部变量 → 难排障 | **核心链单独 `-O3`**；外围转发/控制台仍可 **`-O2`**；靠埋点/监控补可观测性 |
| **体积 / 内存** | 内联+展开 → 镜像变大，容器/多模块成本敏感 | 裸金属资源充裕，体积次要 |
| **浮点语义** | 支付/金额要严 IEEE → **忌 `-Ofast`/`-ffast-math`** | 价差/速率等路径可评估 **可选** `-ffast-math`；金额类仍慎用 |
| **瓶颈在哪** | 慢在 DB / 网络 / 磁盘 / Redis；CPU 常 <1% → O3 **几乎无体感** | 瓶颈就是 **CPU 周期 / 缓存 / 分支**；少一拍可能丢成交 |

**浮点纠正（易混）：** 默认 **`-O3` 并不等于放宽 IEEE754**。真正常放宽标准的是 **`-Ofast`**（含 **`-ffast-math`** 等）。金额业务忌 Ofast；勿把「O3 会乱改浮点」当成默认事实。

**HFT 规范（简）：** 日常学/调 → `-O0` / **`-Og`**；交易核心 Release → **`-O3`**（+ 与生产一致的 `-march`/LTO/PGO）；**不要整仓盲目 O3**。  
**`-O3` 是入场票，不是护城河** — 编译器天花板、负优化、毛刺排查 → [Ch5 §5.1](../../chapter-05-optimizing-performance/notes/section-5.1-优化编译器的能力和局限性.md)

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
3. **`-O3` 字母怎么写？** 相对 `-O2` 多了哪类对延迟有用的优化？为何调试别用 O3？学汇编用啥？  
4. **普通业务为何多用 O2？** HFT 为何只热路径 O3？默认 O3 会破坏 IEEE 吗？  
5. **`.efi` 和本章编译链关系？** — 同样 **C → .o → 链接**；UEFI 链出 **PE** 而非 ELF  

---

← [本章导读](../README.md) · [§3.1 ←](./section-3.1-历史观点.md) · [§3.2.2 →](./section-3.2.2-栈帧.md)
