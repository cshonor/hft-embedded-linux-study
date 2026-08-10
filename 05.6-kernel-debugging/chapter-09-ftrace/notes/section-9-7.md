# 9.7 perf-tools ftrace wrapper

> 🔴 精读

## 本节要点

### perf-tools 简介

perf-tools 是 Brendan Gregg 开发的 ftrace 封装脚本集合，简化常用追踪操作。

### 常用工具

```bash
# 获取 perf-tools
git clone https://github.com/brendangregg/perf-tools.git
cd perf-tools/bin

# 1. 函数调用频率统计
./funccount schedule         # 统计 schedule() 调用次数
./funccount 'vfs_*'         # 通配符统计

# 2. 函数耗时
./funclatency schedule       # schedule() 耗时直方图
./funclatency -m vfs_write   # vfs_write() 耗时 (毫秒)

# 3. 调用路径
./kprobe schedule 'cpu=$cpu' # 追踪 schedule 调用时的 CPU 号

# 4. 系统调用追踪
./syscount                   # 系统调用频率统计
./syssize                    # 系统调用参数大小

# 5. 前端工具
./bashreadelf                # 追踪 ELF 读取
./iolatency                  # I/O 延迟直方图
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** perf-tools 和 trace-cmd 的定位有什么不同？

> trace-cmd 是 tracefs 的完整封装，支持所有 ftrace 功能但接口较复杂。perf-tools 是高层封装，提供常用场景的一键脚本（如 funclatency 直接输出直方图），更易用但功能有限。快速分析用 perf-tools，深度分析用 trace-cmd。


**Q:** perf-tools（Brendan Gregg）中的 perf-tools 对 ftrace 做了什么封装？

> perf-tools 是一组 shell 脚本，封装了常用 ftrace 操作为易用工具。如 `funcgraph schedule` = 设置 function_graph + 过滤 schedule + 运行 + 停止 + 格式化输出。`kprobe -s 'tcp_sendmsg'` = 自动创建 kprobe + 提取参数 + 显示调用栈。降低了 ftrace 的使用门槛。

</details>

## 交叉引用

- [05.6 ch09 ftrace vs eBPF](chapter-09-ftrace/notes/section-9-8.md)
