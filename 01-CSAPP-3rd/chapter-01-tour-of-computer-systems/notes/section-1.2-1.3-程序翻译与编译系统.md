## 1.2–1.3 程序翻译与编译系统

### 1.2 程序被其他程序翻译成不同格式

从 `hello.c` 到能在机器上跑，要经过 **编译系统（compilation system）** 多步翻译 — 每一步都在换 **上下文**（源码 → 汇编 → 机器码 → 可执行）：

```
hello.c  ──预处理器──►  hello.i（展开 #include/#define）
         ──编译器──►    hello.s（汇编文本）
         ──汇编器──►    hello.o（目标文件，机器码 + 符号表）
         ──链接器──►    a.out（可执行，含 CRT、libc 等）
```

| 阶段 | 输入 | 输出 | 工具 |
|------|------|------|------|
| 预处理 | `.c` | `.i` | `cpp` / `gcc -E` |
| 编译 | `.i` | `.s` | `cc1` / `gcc -S` |
| 汇编 | `.s` | `.o` | `as` |
| 链接 | `.o` + 库 | 可执行 | `ld` / `gcc` |

**静态链接 vs 动态链接：** 可执行可嵌入 libc，或运行时加载 `libc.so` — HFT 生产常静态/部分静态以减少依赖与启动不确定性（→ [Ch 7 链接](../../chapter-07-linking/)）。

---

### CSAPP 四步 ↔ 编译器「前端 / 后端」（怎么对照看）

两套划分 **粒度不同**，不是互相矛盾 — 先记 CSAPP 四步搭框架，编译工程课再往 `cc1` 里面拆细。

**编译工程课（龙书等）里的编译器本体：**

```
源码 ──前端 FE──► IR（中间表示）──后端 BE──► 目标代码（汇编或 .o）
         │                              │
    词法/语法/语义                   IR 优化、指令选择、寄存器分配
    （跟语言有关）                   （跟 CPU 有关）
```

**CSAPP / gcc 工具链 — 四步与谁对应：**

| CSAPP 步骤 | 工具 | 编译 FE/BE？ | 在干什么 |
|------------|------|--------------|----------|
| **预处理** | `cpp` | ❌ 不算 FE/BE | 文本层：`#include` / `#define` 展开 → `.i` |
| **编译** | `cc1` | ✅ **FE + BE 都在这里** | `.i` → 词法/语法/语义 → IR → 优化 → **`.s` 汇编** |
| **汇编** | `as` | ⚠️ 编译器外、工具链内 | `.s` 助记符 → `.o` 机器码 + **符号表 + 重定位条目** |
| **链接** | `ld` | ❌ 不算编译器 | 多个 `.o` + 库 → 解析符号、填地址、拼成可执行 |

**一张总图（推荐记在脑子里）：**

```
hello.c
  │  预处理 cpp          ← 编译系统，不是编译器 FE
  ▼
hello.i
  │  cc1 ┌─ 前端：C → AST/IR
  │      └─ 后端：IR → hello.s     ← 编译工程课主要啃这里
  ▼
hello.s
  │  汇编 as                       ← 独立工具，严格说不叫 FE/BE
  ▼
hello.o（有符号 printf 未定义）
  │  链接 ld                       ← 整合资源：符号解析、重定位、静/动态链
  ▼
a.out（能 execve 跑）
```

**容易混的三句话：**

1. **「预处理、编译、汇编是前端」** — 粗记可以，但 **严格说只有 `cc1` 里的前半是 FE**；预处理是 cpp，汇编是 `as`，都不在龙书的 FE/BE 框里。
2. **「链接是后端」** — 口语上指「更贴硬件、系统」，但 **课本里链接器是编译器之外的 toolchain 组件**；后端停在 **吐出汇编或目标文件**。
3. **`gcc -S` 只看到 `.s`** — 因为 FE+BE 都在 `cc1` 里跑完了；**IR 默认不落盘**（`-fdump-tree-*` / `-emit-llvm` 才能看中间产物）。

**和量化 / HFT 的直接相关度（先抓哪块）：**

| 优先级 | 阶段 | 为何 |
|--------|------|------|
| 🔴 先搞懂 | **链接** | 静/动态链、符号解析、`undefined reference`、启动是否拉 `.so` — 实盘环境一致性 |
| 🟡 其次 | **编译 (BE 优化)** | `-O3` / LTO / `-march=native` 改 hot path 指令（→ [Ch 5](../../chapter-05-optimizing-performance/)） |
| 🟡 其次 | **汇编 `.o`** | 懂「有符号、有重定位」→ 链接时在填什么 |
| ⚪ 可后补 | **FE / IR 细节** | 写策略不手写 parser；啃编译工程时再深入 |
| ⚪ 常略过 | **预处理** | 除非用宏做 compile-time 配置 |

**LTO 模糊边界：** `-flto` 时编译只产 **带 IR 的 `.o`**，优化延到 **链接阶段** 才做 — 说明「编译 vs 链接」在现代工具链里可以交叉，但 **概念上仍是 FE → BE → 链接** 三条线。

---

**为何要懂：**

- **优化** — `-O2/-O3`、LTO、PGO 改的是哪一阶段产物
- **调试** — 崩溃栈、符号、行号对应哪层翻译
- **链接错误** — undefined reference、ODR、ABI 不匹配
- **安全/合规** — 可重现构建

---

### 量化场景映射 · 编译链 ↔ 策略生命周期

把 CSAPP 编译链和以后写策略/引擎对照记 — **同一条「从文本到跑起来」的链**：

| 编译链环节 | 在干什么 | 量化 / DeFi 类比 |
|------------|----------|------------------|
| **预处理** `#include` / `#define` | 文本替换、头文件展开 | **策略参数注入**：把 `MAX_POSITION`、`SYMBOL_LIST` 从 config/宏写进编译期或代码 |
| **编译** `.c` → `.s` | 高级逻辑 → 汇编 | **策略逻辑 → 可执行决策**：信号计算、风控规则变成 CPU 会执行的指令 |
| **汇编** `.s` → `.o` | 助记符 → 机器码 | **定稿 hot path**：哪几段汇编是 tick 内必须跑完的 |
| **链接** `.o` + 库 → 可执行 | 拼模块、解析符号 | **集成**：行情解码库 + 订单网关 + 日志 + 你的 alpha 链成一个 binary |
| **加载运行** `execve` | OS 映射进内存、跳 `_start` | **实盘启动**：pin 核、连 feed、进入主循环收行情 |

```
config.yaml / 宏参数     ≈  预处理
策略 C++ 源码            ≈  .c
Release -O3 -march=native ≈  编译+汇编
链 libfix + libmd        ≈  链接
systemd 拉起 pinned 进程  ≈  execve 加载运行
```

**和 1.1 的「上下文」：** 源码里 `price > threshold` 是 **C 语法上下文**；编译后是 **指令上下文**；线上 tick 里是 **协议字段上下文** — 三条链最终在 **同一进程、同一 CPU** 上会合。

---

### 动手实验 · hello.c 走一遍编译链

**1. 准备最小程序**

```c
// hello.c
#include <stdio.h>
int main() {
    printf("hello, world\n");
    return 0;
}
```

**2. 看 gcc 背后调了谁（推荐第一次做）**

```bash
gcc -v hello.c -o hello 2>&1 | less
```

输出里能看到：`cc1`（编译）→ `as`（汇编）→ `collect2`/`ld`（链接），以及 `-L` 库搜索路径。建立直觉：**gcc 是驱动，不是单独一个程序**。

**3. 分阶段产物（建议每步 `-o` 打开看一眼）**

```bash
gcc -E hello.c -o hello.i          # 预处理：stdio.h 展开成几千行
gcc -S hello.c -o hello.s          # 汇编：搜 main、call printf
gcc -c hello.c -o hello.o          # 目标文件：机器码，人眼不可读
file hello.o                       # ELF 64-bit relocatable
gcc hello.o -o hello               # 链接：拉进 crt、libc
./hello
```

**4. 看机器码与反汇编（和 1.1「同一比特不同上下文」呼应）**

```bash
objdump -d hello.o | less          # 未链接：call 还是 rel 占位
objdump -d hello | less            # 链接后：完整 main 反汇编
```

**5. 和量化相关的两个延伸（可选）**

```bash
gcc -O0 -S hello.c -o hello-O0.s
gcc -O3 -S hello.c -o hello-O3.s
diff -u hello-O0.s hello-O3.s      # 看优化如何改「指令上下文」

gcc -g hello.c -o hello && gdb ./hello
# (gdb) break main → run → disassemble  # 源码行 ↔ 汇编 ↔ 地址
```

**Checklist（做完打勾）：**

- [ ] 能说出 `.i` / `.s` / `.o` / 可执行各是什么上下文
- [ ] `gcc -v` 里见过 `cc1`、`as`、`collect2`
- [ ] 用 `objdump -d` 看过 `main` 至少一条 `call`

---

### HFT 构建注意

- **Release flags 在 CI 与生产一致** — `-march=native`、LTO、`-DNDEBUG`
- **链接方式** — whole-archive 静态库 vs `dlopen` 插件 → 启动延迟与页 fault（→ [Ch 7](../../chapter-07-linking/)、[Ch 9](../../chapter-09-virtual-memory/)）
- **行情解码** — 协议 `.h` 里 struct layout 要和 spec 逐字段对齐；改 spec = 改「协议上下文」，常要 **版本号分支**（回连 1.1）

---

← [本章导读](../README.md)
