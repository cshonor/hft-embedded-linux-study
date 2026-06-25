# 2. 编译型语言 (C, C++, Rust, Go…)

### 追踪方式

| 手段 | 说明 |
|------|------|
| **`uprobe` / `uretprobe`** | 二进制任意函数入口/返回 |
| **USDT** | 源码预埋静态探针 — **首选**（稳定、低开销） |

```bash
# uprobe 示例（需符号未 strip）
sudo bpftrace -e 'uprobe:/path/to/bin:my_func { @[ustack] = count(); }'
```

→ USDT 总论：[Ch 2 § USDT](../../chapter-02-technology-background/)

### 符号表 (Symbols)

BPF 用 **`.symtab` / `.dynsym`** 将地址 → 函数名。

| 生产现状 | 对策 |
|----------|------|
| **`strip(1)` 剥离符号** | 发布保留符号分区，或安装 **`debuginfo`/`dbgsym` 包** |
| 仅动态符号 | `readelf -s` / `nm` 确认 |

**HFT：** 策略 **`.so` 发布包** 建议保留 **至少动态符号 + debuginfo 分离包** — 否则 `profile`/`offcputime` 栈为 `[unknown]`。

### 调用栈 (Frame Pointer)

栈 walk 依赖 **帧指针链**（x86-64 `RBP`）— 见 [Ch 2 § 栈遍历](../../chapter-02-technology-background/)。

| 编译选项 | 效果 |
|----------|------|
| **默认 `-fomit-frame-pointer`** | 栈 **断裂** / `[unknown]` |
| **`-fno-omit-frame-pointer`** | 火焰图可用 |
| **`-g` + DWARF** | 更准但更慢（可选） |

```bash
# GCC/Clang
CFLAGS="-fno-omit-frame-pointer -g"
```

**HFT 构建链：** 与 [SysPerf Ch 6 CPU](../../../02-Systems-Performance-2nd/chapter-06-cpus/) **FPO 讨论同构** — 性能与可观测性 trade-off，热路径可 **USDT 替代高频 uprobe**。

### Golang 特殊陷阱 ⚠️

Go 是编译型，但 **动态栈** + 独特 ABI：

| 做法 | 风险 |
|------|------|
| **`uretprobe` on Go 函数** | **极危险** — 破坏栈，**崩溃/数据损坏** |
| `uprobe` | 相对可行，仍须谨慎、低频 |
| 官方方向 | **USDT**、`runtime/trace`、Go pprof |

**HFT：** 共置 **Go 微服务** 用 **应用内 metrics + pprof** 为主；BPF 仅 **syscall/网络层**（Ch 10）辅助。

### Rust

与 C/C++ 同类：**ELF + frame pointer**；名称 **mangling**（demangle 用 `rustfilt`）。无 GC — 栈行为比 Go 简单。

→ 本仓库：[16-Rust-Quant-Trading-Guide](../16-Rust-Quant-Trading-Guide/)

### C++ 注意点

| 问题 | 说明 |
|------|------|
| **Name mangling** | `c++filt` / bpftrace 符号 demangle |
| **成员函数** | **`arg0` 常是 `this`** — 实参从 `arg1` 起 |
| **内联** | 热点可能 **无独立符号** — 看编译单元或 LTO 行为 |

---
