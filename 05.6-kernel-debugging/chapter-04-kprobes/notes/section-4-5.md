# 4.5 perf probe 与 Kprobes 的关系

> 🔴 精读

## 本节要点

### perf probe 架构

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

### perf probe 的优势

| 特性 | kprobe_events (手动) | perf probe |
|------|---------------------|------------|
| 符号解析 | 仅函数名 | 函数名 + 行号 + 变量名 |
| 参数提取 | 手动指定寄存器/偏移 | 自动从 DWARF 识别 |
| 行号探针 | 不支持 | `schedule:42` 可行 |
| 批量操作 | 逐条 | 支持 `--add` 批量 |
| 依赖 | 仅 debugfs | 需要调试符号 (vmlinux) |

### 典型用法

```bash
# 在函数特定行插入探针
perf probe --add '__kmalloc:12 size bytes_req'

# 查看可用探针点（需要调试符号）
perf probe --line '__kmalloc'

# 使用 perf record 追踪
perf record -e probe:__kmalloc -aR sleep 5
perf script  # 查看结果

# 与 callchain 配合（查看调用栈）
perf record -e probe:__kmalloc -g -aR sleep 5
perf report -g graph
```

### HFT 关联

perf probe 结合 perf record/report 提供 HFT 内核函数耗时分析的完整工作流：定位热点函数 → 插入探针 → 记录 → 分析。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** perf probe 需要什么前提条件？

> 需要内核调试符号（CONFIG_DEBUG_INFO=y，有 vmlinux 文件）。DWARF 调试信息用于解析变量名和行号。如果没有调试符号，perf probe 退化为只能按函数名注册（等同于直接写 kprobe_events）。

**Q2:** perf probe 和 perf record 的关系是什么？

> perf probe 负责**注册/注销** kprobe 探针，perf record 负责**收集**探针触发的事件。工作流：1) `perf probe --add` 注册探针 → 2) `perf record -e probe:xxx` 收集事件 → 3) `perf report` 分析 → 4) `perf probe --del` 清理。

</details>
