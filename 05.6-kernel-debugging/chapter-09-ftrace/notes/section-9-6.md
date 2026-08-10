# 9.6 KernelShark：GUI 前端

> 🔴 精读

## 本节要点

### KernelShark 概述

KernelShark 是 trace-cmd 的 GUI 前端，可视化展示 ftrace 事件时间线。

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

### 功能

- 时间线视图：CPU 上的事件按时间排列
- 过滤：按进程、CPU、事件类型过滤
- 缩放：鼠标滚轮缩放时间轴
- 关联：点击事件查看详细信息（寄存器、调用栈）

### HFT 关联

KernelShark 可视化展示调度切换和中断，帮助 HFT 开发者直观理解交易线程的调度行为和中断干扰。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KernelShark 能在树莓派 5 上运行吗？

> 可以但需要桌面环境（X11/Wayland）。树莓派 5 支持 GUI，但建议在 PC 上分析 .dat 文件（将树莓派上收集的 trace.dat 拷贝到 PC 上用 KernelShark 打开），因为 GUI 分析对 CPU/内存要求较高。

</details>
