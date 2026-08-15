## 15.2 bpftrace

### 是什么

**高级追踪语言** — 语法类似 awk/C，**单行命令** 极快。

```bash
# 统计 read syscall 调用次数
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_read { @ = count(); }'

# 按进程统计 open 路径
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_openat /pid==$(pidof strategy)/ { @[comm] = count(); }'

# uprobe 用户函数延迟直方图
sudo bpftrace -e 'uprobe:/path/strategy:decode { @start[tid] = nsecs; }
    uretprobe:/path/strategy:decode /@start[tid]/ { @lat = hist(nsecs - @start[tid]); delete(@start[tid]); }'
```

### 单行命令优势

| 场景 | bpftrace |
|------|----------|
| **即兴假设验证** | 「是不是这个内核函数慢？」— 一行 kprobe |
| **定制 filter** | pid、comm、栈、直方图 |
| **USDT** | `usdt:...` 探针 |
| **教学/探索** | 比写 BCC Python 快 10× |

**本仓库：** [附录 C bpftrace 单行命令](../../appendix-C-bpftrace单行命令.md) — SysPerf 配套备忘。

### bpftrace 适用场景

| 适合 | 不适合 |
|------|--------|
| Ad hoc 根因、一次性调查 | 需复杂 GUI、长期产品化 |
| 快速 kprobe/uprobe 实验 | 极老内核无 bpftrace |
| 与 BCC 工具 **组合** | 替代所有 BCC（不必） |

→ [17-BPF ch05 bpftrace](../../../16-bpf-observability/chapter-05-bpftrace/)

---


### 常见陷阱

1. bpftrace 生产直接跑自定义脚本——自定义 kprobe 可能加载失败或开销过大，应先 staging 验证
2. bpftrace 每事件输出——高频事件（sched_switch）每条 print 打爆 CPU，应用 map 聚合
3. bpftrace 当 BCC 用——复杂状态机/多 map 协作用 BCC Python，bpftrace DSL 表达力有限

<details>
<summary>自测题（点击展开）</summary>

1. bpftrace 自定义脚本在生产环境的风险？
   <details><summary>答</summary>kprobe 可能加载失败/开销过大——应先在 staging 验证加载和开销</details>
2. 为什么 bpftrace 高频事件不能每条 print？
   <details><summary>答</summary>sched_switch 每秒上千次——每条送到用户态会打爆 CPU，应用 map histogram 聚合</details>
3. bpftrace 和 BCC 的分工？
   <details><summary>答</summary>bpftrace 适合即兴单行/简单脚本；复杂多事件状态机用 BCC Python</details>

</details>


---

← [本章导读](../README.md)
