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


### 常见陷阱

1. **只关注 kprobe/uprobe 忽视其他事件源** — bpftrace 支持 tracepoint、USDT、interval、profile、hardware PMU、software event 等；不同问题应选不同事件源
2. **混淆 interval 和 profile 探针** — interval:s:5 每 5 秒触发一次（固定间隔），profile:hz:99 每秒采样 99 次（CPU 采样）；用途完全不同
3. **忽视 BEGIN/END 的初始化和收尾作用** — BEGIN 用于初始化变量和输出表头，END 用于收尾打印和清理；不利用这两个探针会导致输出格式混乱

<details>
<summary>📝 自测题（点击展开）</summary>

1. **bpftrace 支持哪些事件源类型？**

   <details>
   <summary>参考答案</summary>

   (1) kprobe/kretprobe——内核函数入口/返回；(2) uprobe/uretprobe——用户态函数；(3) tracepoint——内核静态探针；(4) USDT——用户态静态探针；(5) profile:hz:N——CPU 定时采样；(6) interval:s:N——固定间隔触发；(7) BEGIN/END——脚本启停；(8) hardware/software PMU 事件。

   </details>

2. **profile:hz:99 和 interval:s:5 有什么区别？**

   <details>
   <summary>参考答案</summary>

   profile:hz:99：每秒在当前 CPU 上采样 99 次（基于定时器中断），用于 CPU 热点分析——采样到的是「此刻 CPU 在执行什么」。interval:s:5：每 5 秒在某个 CPU 上触发一次，用于定时输出汇总——不关心 CPU 在做什么，只用作周期性触发器。

   </details>

3. **BEGIN 和 END 探针在 bpftrace 脚本中有什么用途？**

   <details>
   <summary>参考答案</summary>

   BEGIN：脚本启动时执行一次，用于初始化（如打印表头、设置时间基准 `@start = nsecs`）。END：脚本退出（Ctrl-C）时执行，用于收尾输出（如 `print(@map)` 自定义格式、计算总耗时）。不使用 BEGIN/END 时 Map 默认在退出时自动打印，但格式不可控。

   </details>

</details>

---
