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
7. 字节码送入内核，经** BPF 验证器**检查；
8. 事件被启用，BPF 程序**挂载到事件**；
9. BCC 程序经**映射表或 perf 事件缓冲区**读取数据。

## HFT 关联

- 理解这 9 步 = 理解 BCC 工具**启动慢**（每次运行都 Clang 编译）而**运行快**（JIT 后的 BPF 字节码）的原因——生产上用 libbpf-tools 的 CO-RE 版本可消除运行时编译（启动即秒级）。
- 排障时定位卡在哪一步：编译错误在第 2–5 步，`Permission denied` 在第 7 步（验证器），挂载失败在第 8 步。

## 陷阱

- ⚠️ BPF C 里**直接解引用指针**能编译过，是因为改写器帮你转成了 `bpf_probe_read()`——理解这一点才能看懂内核拒绝时的报错。
- ⚠️ bpftrace 不是"另一个轮子"：它底层就跑在 BCC 的 libbcc 上（新版已迁 libbpf），生态是同源的。

<details>
<summary>自测题</summary>

1. BCC 改写器（rewriter）做了什么？
   <details><summary>答案</summary>预处理 BPF C，把内存引用/指针解引用替换为 `bpf_probe_read()` 调用。</details>

2. 说出装载流程中"验证器"所在的步骤编号与作用。
   <details><summary>答案</summary>第 7 步：BPF 字节码送入内核后由验证器做安全检查（内存访问、循环、栈边界）。</details>

3. 为什么 USDT 在 BCC 中是独立对象？
   <details><summary>答案</summary>必须挂到进程/路径，且部分探针需设置进程内信号量激活。</details>
</details>
