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

→ [04-BPF ch05 bpftrace](../../../04-BPF-Performance-Tools/chapter-05-bpftrace/)

---


---

← [本章导读](../README.md)
