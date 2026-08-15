# KernelShark：GUI 前端

> 🔴 精读

## 概念详解

### KernelShark 是什么

KernelShark 是 trace-cmd 的 GUI 前端，可视化展示 ftrace 事件时间线。它将追踪数据以图形化方式呈现，便于分析多 CPU 上的事件分布和时序关系。

### 使用方法

```bash
# 安装
sudo apt install kernelshark

# 收集数据
trace-cmd record -e sched -e irq sleep 5
# 生成 trace.dat

# 打开 GUI
kernelshark trace.dat
```

### 主要功能

| 功能 | 说明 | 使用场景 |
|------|------|---------|
| 时间线视图 | CPU 上的事件按时间排列 | 查看事件分布 |
| 过滤 | 按进程、CPU、事件类型过滤 | 聚焦特定数据 |
| 缩放 | 鼠标滚轮缩放时间轴 | 精确查看时间段 |
| 关联 | 点击事件查看详细信息 | 分析单个事件 |
| 图形化 | 颜色区分事件类型 | 快速识别模式 |
| 导出 | 导出为文本/图片 | 生成报告 |

### KernelShark 界面说明

```
┌─────────────────────────────────────────┐
│ 菜单栏: File/Edit/Filter/View/Help       │
├─────────────────────────────────────────┤
│ CPU 列表:                                │
│   CPU 0: ████░░░░████░░░░██░░░██        │ ← 事件时间线
│   CPU 1: ░░░░████░░░░██░░░░████░░       │
│   CPU 2: ██░░░░░░░░██████░░░░░░░░       │
│   CPU 3: ░░░░██░░░░░░░░░░░░████░░       │
├─────────────────────────────────────────┤
│ 事件列表:                                │
│   时间    CPU  进程    事件    详情      │
│   123.456  0   trade   switch  ...      │
│   123.457  2   irq     entry   eth0     │
└─────────────────────────────────────────┘
```

### 典型分析流程

```
1. 用 trace-cmd 采集数据
2. 用 KernelShark 打开 .dat 文件
3. 设置过滤（如只看 trade_app 进程）
4. 在时间线上找到延迟毛刺
5. 点击事件查看详细信息
6. 分析前后事件关联
```

### 过滤功能

```bash
# 在 KernelShark GUI 中设置过滤:
# - 按进程: 只显示 trade_app 的事件
# - 按CPU: 只显示 CPU 2 的事件
# - 按事件类型: 只显示 sched_switch
# - 按时间范围: 只显示 10:00-10:05 的事件
```

### HFT 关联应用

KernelShark 可视化展示调度切换和中断，帮助 HFT 开发者直观理解交易线程的调度行为和中断干扰。

```bash
# HFT 分析流程:
# 1. 采集数据
trace-cmd record -e sched -e irq -e net sleep 30

# 2. 在 PC 上用 KernelShark 打开
kernelshark trace.dat

# 3. 分析:
#    - 交易线程被调度出去的时间点
#    - 中断是否在交易关键路径上发生
#    - CPU 之间的事件关联
#    - 延迟毛刺前后的系统状态
```

### KernelShark vs 命令行分析

| 方面 | KernelShark | 命令行 (trace-cmd report) |
|------|------------|--------------------------|
| 时间线可视化 | 直观图形 | 文本列表 |
| 交互性 | 点击/缩放/过滤 | grep/awk |
| 多 CPU 关联 | 一目了然 | 需要手动排序 |
| 自动化 | 不支持 | 脚本化 |
| 适用 | 交互分析 | 批量处理 |

### 在树莓派上使用

```bash
# 树莓派 5 支持桌面环境
# 但建议在 PC 上分析 .dat 文件
# (将树莓派上采集的 trace.dat 拷贝到 PC)

# 树莓派上采集
trace-cmd record -e sched -e irq sleep 10
scp trace.dat user@pc:/tmp/

# PC 上分析
kernelshark /tmp/trace.dat
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KernelShark 能在树莓派 5 上运行吗？

> 可以但需要桌面环境。建议在 PC 上分析 .dat 文件（将树莓派上采集的 trace.dat 拷贝到 PC 上用 KernelShark 打开），因为 GUI 分析对 CPU/内存要求较高。

**Q2:** KernelShark 适合什么场景？命令行工具和它如何配合？

> KernelShark 适合需要时间线可视化的场景——在 GUI 中看多个 CPU 上事件的时间分布、任务切换模式、延迟毛刺。典型工作流：trace-cmd record 采集 → trace-cmd report 初步检查 → KernelShark 深入可视化分析。

**Q3:** KernelShark 的过滤功能有哪些？

> 按进程（只显示特定进程的事件）、按 CPU（只显示特定 CPU）、按事件类型（只显示 sched_switch 等）、按时间范围。过滤后只显示感兴趣的数据。

**Q4:** 为什么建议在 PC 上而不是树莓派上运行 KernelShark？

> (1) GUI 分析对 CPU/内存要求较高，树莓派可能卡顿；(2) PC 屏幕更大，适合查看时间线；(3) trace.dat 文件不大，拷贝到 PC 很快。在树莓派上只做采集，在 PC 上做分析。

**Q5:** HFT 延迟分析中 KernelShark 的核心价值是什么？

> 可视化展示交易线程在时间线上的调度行为——何时被调度出去、何时被唤醒、中断在何时发生。通过图形化时间线，可以直观看到延迟毛刺发生时的系统状态，找到根本原因。

</details>

## 交叉引用

- [05.6 ch09 trace-cmd 命令行前端](../../chapter-09-ftrace/notes/05-trace-cmd.md)
- [05.6 ch09 Ftrace 架构与 tracefs](../../chapter-09-ftrace/notes/01-ftrace-architecture-tracefs.md)
- [05.6 ch09 perf-tools ftrace wrapper](../../chapter-09-ftrace/notes/07-perf-tools-ftrace.md)
