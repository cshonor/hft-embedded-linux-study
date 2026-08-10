# 9.5 trace-cmd：命令行前端

> 🔴 精读

## 本节要点

### trace-cmd 基本用法

```bash
# 安装
sudo apt install trace-cmd

# 基本用法
trace-cmd record -e sched schedule_switch -e irq irq_handler_entry sleep 5
trace-cmd report > trace.txt

# 函数追踪
trace-cmd record -p function -l schedule sleep 1
trace-cmd report

# function_graph
trace-cmd record -p function_graph -l my_driver_write sleep 1
trace-cmd report

# kprobe
trace-cmd record -e p:my_probe schedule sleep 1
trace-cmd report

# 过滤
trace-cmd record -e sched_switch -f 'prev_pid == 1234' sleep 5
```

### trace-cmd 优势

| 特性 | 直接操作 tracefs | trace-cmd |
|------|-----------------|-----------|
| 事件配置 | 手动 echo | 命令行参数 |
| 数据保存 | cat trace > file | 自动保存 .dat |
| 多实例 | 复杂 | trace-cmd stream |
| 报告生成 | 手动解析 | trace-cmd report |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** trace-cmd 生成的 .dat 文件如何分析？

> 用 `trace-cmd report trace.dat` 转为可读文本。也可以用 KernelShark（GUI 工具）打开 .dat 文件进行可视化分析。trace-cmd 的 .dat 格式是二进制的，包含完整的事件数据和元数据。

</details>
