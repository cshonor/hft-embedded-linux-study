# 4.5 perf probe 与 Kprobes 的关系

> 🔴 精读 · Part 2: Instrumentation & Memory Debugging

## 本节要点

perf probe 是 kprobe_events 的高级封装，利用 DWARF 调试信息提供行号级探针和自动参数提取。

## perf probe 架构

```
用户命令          工具层               内核层
┌────────┐    ┌──────────┐     ┌──────────────────┐
│perf    │───→│perf probe│────→│kprobe_events     │
│probe   │    │(DWARF    │     │(kprobe 注册/注销) │
│--add   │    │ 解析)    │     │                  │
└────────┘    └──────────┘     └──────────────────┘
                                    ↓
                              ┌──────────┐
                              │ kprobe   │
                              │ engine   │
                              └──────────┘
```

## perf probe 的优势

| 特性 | kprobe_events (手动) | perf probe |
|------|---------------------|------------|
| 符号解析 | 仅函数名 | 函数名 + 行号 + 变量名 |
| 参数提取 | 手动指定寄存器/偏移 | 自动从 DWARF 识别 |
| 行号探针 | 不支持 | `schedule:42` 可行 |
| 批量操作 | 逐条 | 支持 `--add` 批量 |
| 依赖 | 仅 debugfs | 需要调试符号 (vmlinux) |
| 参数验证 | 无 | 编译时检查类型 |

## 典型用法

```bash
# 在函数特定行插入探针
perf probe --add '__kmalloc:12 size bytes_req'

# 查看可用探针点（需要调试符号）
perf probe --line '__kmalloc'
# 输出:
# __kmalloc@aabbccdd:
#       0  size_t size;
#       1  gfp_t flags;
#       2  {
#       3      struct kmem_cache *s;

# 查看可用变量
perf probe --vars '__kmalloc'
# 输出:
# @<__kmalloc+0>
#         size_t  size
#         gfp_t   flags

# 使用 perf record 追踪
perf record -e probe:__kmalloc -aR sleep 5
perf script  # 查看结果

# 与 callchain 配合（查看调用栈）
perf record -e probe:__kmalloc -g -aR sleep 5
perf report -g graph
```

## perf probe + perf record 工作流

```bash
# Step 1: 注册探针
perf probe --add 'schedule'
perf probe --add 'schedule%return'
perf probe --add '__kmalloc size'

# Step 2: 查看已注册探针
perf probe -l
# probe:schedule      (on schedule)
# probe:schedule__return (on schedule%return)
# probe:__kmalloc     (on __kmalloc with size)

# Step 3: 记录
perf record -e probe:schedule -e probe:__kmalloc -aR sleep 10

# Step 4: 分析
perf script   # 时间线视图
perf report   # 热点统计

# Step 5: 清理
perf probe --del 'schedule'
perf probe --del 'schedule%return'
perf probe --del '__kmalloc'
```

## perf probe 高级用法

```bash
# 条件探针（只在满足条件时触发）
perf probe --add '__kmalloc size' --filter 'size > 65536'

# 带调用栈
perf probe --add 'schedule' --call-graph dwarf

# 批量注册
perf probe --add 'schedule' --add '__kmalloc size' --add 'kfree'

# 模块函数
perf probe --add 'my_module:my_func'

# 行号范围
perf probe --line '__kmalloc:10-20'
```

## perf probe vs kprobe_events 对比

```bash
# kprobe_events: 手动指定参数位置
echo 'p:my_open do_sys_openat2 dfd=%arg1 file=+0(%arg2):string' > kprobe_events

# perf probe: 自动解析参数
perf probe --add 'do_sys_openat2 dfd file:string'
# perf probe 自动从 DWARF 识别 dfd 和 file 参数的寄存器位置

# kprobe_events: 不支持行号
# perf probe: 支持行号
perf probe --add 'vfs_write:642 count'
```

## HFT 关联

perf probe 结合 perf record/report 提供 HFT 内核函数耗时分析的完整工作流：

1. **定位热点**：`perf record -g` + `perf report` 找到耗时最多的函数
2. **插入探针**：`perf probe --add 'hot_func'` 在热点函数注册探针
3. **记录详情**：`perf record -e probe:hot_func -g` 收集调用栈
4. **分析结果**：`perf report -g graph` 查看调用链和耗时

```bash
# HFT 延迟溯源完整工作流
# 1. 找热点
perf record -g -a sleep 10
perf report

# 2. 在热点函数插入探针
perf probe --add 'hft_process_packet'
perf probe --add 'hft_process_packet%return'

# 3. 记录耗时
perf record -e probe:hft_process_packet -e probe:hft_process_packet__return -a sleep 10

# 4. 分析
perf script  # 查看每次调用的耗时
perf report  # 统计分析
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** perf probe 需要什么前提条件？

> 需要内核调试符号（CONFIG_DEBUG_INFO=y，有 vmlinux 文件）。DWARF 调试信息用于解析变量名和行号。如果没有调试符号，perf probe 退化为只能按函数名注册（等同于直接写 kprobe_events）。

**Q2:** perf probe 和 perf record 的关系是什么？

> perf probe 负责**注册/注销** kprobe 探针，perf record 负责**收集**探针触发的事件。工作流：1) `perf probe --add` 注册探针 → 2) `perf record -e probe:xxx` 收集事件 → 3) `perf report` 分析 → 4) `perf probe --del` 清理。

**Q3:** perf probe 和直接写 kprobe_events 相比有什么优势？

> perf probe 自动解析行号和变量名（需要 DEBUG_INFO），不需要手动查寄存器映射。例如 `perf probe --add "vfs_write:642 size"` 自动找到 vfs_write 第642行并提取 size 变量。直接写 kprobe_events 需要手动指定偏移和寄存器。

**Q4:** perf probe --line 的作用是什么？

> `perf probe --line 'function'` 显示函数的源码和对应的行号，帮助确定在哪个位置插入探针。例如 `perf probe --line '__kmalloc'` 显示 `__kmalloc` 函数的源码行号，然后可以用 `perf probe --add '__kmalloc:12'` 在第 12 行插入探针。

**Q5:** perf probe --filter 和 kprobe_events filter 有什么区别？

> 两者功能相似但层次不同。kprobe_events filter 在内核中执行（每个事件都检查）。perf probe --filter 也是写入内核 filter，但 perf probe 还支持在用户空间过滤（perf record --filter）。内核 filter 开销更低。

</details>

## 交叉引用

- [05.6 ch04 kprobes 架构](../../chapter-04-kprobes/notes/01-kprobes-architecture.md)
- [05.6 ch04 动态注册](../../chapter-04-kprobes/notes/04-dynamic-registration-sysfs.md)
- [05.6 ch04 kprobes vs eBPF](../../chapter-04-kprobes/notes/06-kprobes-ebpf.md)
