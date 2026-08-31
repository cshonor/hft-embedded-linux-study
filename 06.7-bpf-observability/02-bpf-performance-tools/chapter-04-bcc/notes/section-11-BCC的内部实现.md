# 4.11 BCC 的内部实现

> 库本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.11 节（印刷 p127–128）

## 内容详解

### BCC 的组成

| 组成 | 职责 |
|------|------|
| **C++ 前端 API** | 编制内核态 BPF C 程序；含**预处理宏：把内存引用改写为 `bpf_probe_read()` 调用**（未来内核含其变体） |
| **C++ 后端驱动** | ① 用 Clang/LLVM 编译 BPF 程序；② 装载 BPF 程序到内核；③ 挂载到事件；④ 读写 BPF 映射表 |
| **语言前端** | Python、C++、Lua |

C++ 组件编译为 **libbcc** 与 **libbcc_bpf** 两个库——**其他软件（如 bpftrace）也直接复用它们**。libbcc_bpf 代码来自 Linux 内核源码树 `tools/lib/bpf`（最早正是从 BCC 捐过去的，即后来的 libbpf）。

### 图 4-5：编译与挂载全景

```
用户态                          内核态
Python: BPF(text=...) ──改写器──▶ Clang ──▶ LLVM ──▶ BPF 字节码
                                                     │ bpf_prog_load()
                                                     ▼
                              验证器 ──▶ 挂载到 kprobes/uprobes/
                                          tracepoints/perf_events
Python: Table / USDT 对象 ◀──映射表/缓冲区──  bpf_create_map()、perf buffer
print_log2_hist() ◀─────── perf_reader_poll()
```

Python 侧的 `BPF`、`Table`、`USDT` 对象是对 libbcc/libbcc_bpf 的**封装**。

### Table 对象：两种等价写法（Python 魔术方法）

```python
counts = b.get_table("counts")
counts = b["counts"]      # __getitem__，完全等价
```

### USDT 为何独立成对象

初始化时**必须挂载到进程 ID 或路径**；且有些 USDT 需在进程映像中**设置信号量**激活——应用程序靠信号量决定是否为探针准备参数，未激活时作为性能优化直接跳过（呼应 2.10）。

### BCC 装载并插桩的 9 步流程

1. 创建 Python `BPF` 对象，传入 BPF C 程序；
2. **BCC 改写器**预处理：内存访问替换为 `bpf_probe_read()`；
3. **Clang** 把 BPF C 编译为 LLVM IR；
4. **BCC codegen** 按需注入额外 LLVM IR；
5. **LLVM** 把 IR 编译为 BPF 字节码；
6. 若用到映射表则创建之（`bpf_create_map()`）;
7. 字节码送入内核，经 **BPF 验证器**检查；
8. 事件被启用，BPF 程序**挂载到事件**；
9. BCC 程序经**映射表或 perf 事件缓冲区**读取数据。

### 按报错定位 9 步中的卡点（排障速查）

9 步流程同时是一张**故障定位表**——每类报错对应一个步骤区间：

| 症状 | 卡在哪步 | 定位动作 |
|---|---|---|
| Clang fatal error / 找不到头文件 | 2–5（编译） | 看 stderr；查 headers 是否安装/匹配 |
| `bpf_prog_load: Operation not permitted` | 7（验证器/权限） | `dmesg \| tail` 看验证器拒绝理由；查 perf_event_paranoid |
| `Attaching` 后无输出 | 8（挂载）或过滤 | 确认探针存在（-l 列出）；放宽 filter |
| 输出有但数值怪 | 9（读取）或程序逻辑 | 对照 /proc 真值校验 |
| 启动极慢 | 2–5（每次都编译） | 换 libbpf-tools 的 CO-RE 版 |

**`dmesg` 是验证器的喉舌**：verifier 拒绝会把完整的原因（哪条指令、哪个寄存器状态不合法）打进内核日志——BPF 工具调试的 90% 从 `dmesg | tail -30` 开始。

### 改写器（rewriter）做了什么——为什么 BCC 的 C"看起来能随便解引用"

BPF 程序本不允许直接解引用任意指针（verifier 拒绝），但 BCC 工具源码里满眼 `bpf_get_current_task()->pid` 这种写法——秘密在改写器：它在编译前把内存访问**自动重写**为 `bpf_probe_read()` 调用（带安全拷贝语义）。

这解释了两个现象：

- BCC 的 BPF C 是"**方言 C**"——比标准 BPF C 更宽容，迁移到 libbpf/CO-RE 时这些语法要手工改回显式 `bpf_probe_read_kernel()`（BCC→libbpf 迁移的主要工作量来源）
- 同一段代码，BCC 下能过、libbpf 下被拒——不是内核退步，是改写器不再代劳

## HFT 关联

- 理解这 9 步 = 理解 BCC 工具**启动慢**（每次运行都 Clang 编译）而**运行快**（JIT 后的 BPF 字节码）的原因——生产上用 libbpf-tools 的 CO-RE 版本可消除运行时编译（启动即秒级）。
- 排障时定位卡在哪一步：编译错误在第 2–5 步，`Permission denied` 在第 7 步（验证器），挂载失败在第 8 步。
- 自研工具的 BCC→libbpf 迁移预算：主要花在改写器方言的逆向（隐式 probe_read 改显式）——评估老脚本转 CO-RE 时按"每百行 C 半天"量级起步。

## 陷阱

- ⚠️ BPF C 里**直接解引用指针**能编译过，是因为改写器帮你转成了 `bpf_probe_read()`——理解这一点才能看懂内核拒绝时的报错。
- ⚠️ bpftrace 不是"另一个轮子"：它底层就跑在 BCC 的 libbcc 上（新版已迁 libbpf），生态是同源的。
- ⚠️ 验证器拒绝的完整理由在 dmesg，不在工具的 stderr——工具通常只回显一行摘要，细节要进内核日志找。

<details>
<summary>自测题</summary>

1. BCC 改写器（rewriter）做了什么？
   <details><summary>答案</summary>预处理 BPF C，把内存引用/指针解引用替换为 `bpf_probe_read()` 调用。</details>

2. 说出装载流程中"验证器"所在的步骤编号与作用。
   <details><summary>答案</summary>第 7 步：BPF 字节码送入内核后由验证器做安全检查（内存访问、循环、栈边界）。</details>

3. 为什么 USDT 在 BCC 中是独立对象？
   <details><summary>答案</summary>必须挂到进程/路径，且部分探针需设置进程内信号量激活。</details>

4. 工具报 `Operation not permitted`，按 9 步流程该查什么？
   <details><summary>答案</summary>第 7 步（验证器/权限）：dmesg 看验证器拒绝理由、查 perf_event_paranoid 与 capability——验证器的完整拒绝原因只打在内核日志里。</details>

5. 为什么同一段 BPF C 在 BCC 下通过、libbpf 下被拒？
   <details><summary>答案</summary>BCC 改写器自动把指针解引用改写成安全的 bpf_probe_read()（方言 C 的宽容）；libbpf 没有改写器，必须显式写 helper 调用——迁移时这些隐式转换全部要手工改。</details>
</details>
