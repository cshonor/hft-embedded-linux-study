# 3. 全栈事件源 (Probes)

bpftrace **可见性极高** — 同一套语法可挂多种事件源：

| 类型 | 前缀/形式 | 说明 |
|------|-----------|------|
| **kprobe** | `kprobe:func` | 内核函数入口 |
| **kretprobe** | `kretprobe:func` | 内核函数返回 |
| **uprobe** | `uprobe:path:func` | 用户态函数入口 |
| **uretprobe** | `uretprobe:path:func` | 用户态函数返回 |
| **tracepoint** | `tracepoint:cat:event` | 内核静态追踪点（稳定、推荐） |
| **usdt** | `usdt:path:probe` | 用户态静态探针 |
| **profile** | `profile:hz:99` | 定时 CPU 采样 |
| **interval** | `interval:s:1` | 定时在用户态执行动作 |
| **software** | `software:faults:1000` | 软 PMU 事件 |
| **hardware** | `hardware:cache-misses:1000` | 硬 PMU 事件 |

**通配符：** 逗号绑定多探针；`kprobe:vfs_*` 匹配所有 `vfs_` 前缀内核函数（注意开销）。

```bash
# 多探针
bpftrace -e 'kprobe:vfs_read,kprobe:vfs_write { @[comm] = count(); }'

# tracepoint（字段名因内核版本而异，先用 bpftrace -l 列出）
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { @ = count(); }'
```

→ 探针原理：[Ch 2 § 插桩](../../chapter-02-technology-background/)

---
