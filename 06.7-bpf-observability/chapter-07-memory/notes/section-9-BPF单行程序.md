# 7.4 BPF 单行程序

> 底本：《BPF之巅》第 7 章 内存，7.4 节（印刷 p288–289）。

## 7.4.1 BCC 版

| 任务 | 命令 |
|------|------|
| 按用户栈统计堆扩展（brk） | `stackcount -u t:syscalls:sys_enter_brk` |
| 按用户栈统计缺页错误 | `stackcount -u t:exceptions:page_fault_user` |
| 统计 vmscan 操作 | `funccount t:vmscan:**` |
| 按进程展示 hugepage_madvise() | `trace hugepage_madvise` |
| 统计页迁移事件 | `funccount t:migrate:mm_migrate_pages` |
| 统计页压缩事件 | `trace t:compaction:mm_compaction_begin` |

## 7.4.2 bpftrace 版

```bash
# 按用户栈统计堆扩展（brk）
bpftrace -e 't:syscalls:sys_enter_brk { @[ustack, comm] = count(); }'

# 按进程统计缺页错误
bpftrace -e 'software:page-fault:1 { @[comm] = count(); }'

# 按用户栈统计缺页错误
bpftrace -e 'software:page-fault:1 { @[ustack, comm] = count(); }'

# 统计 vmscan 操作
bpftrace -e 't:vmscan:** { @[probe] = count(); }'

# 按进程展示 hugepage_madvise()
bpftrace -e 'kprobe:hugepage_madvise { printf("%s by PID %d\n", probe, pid); }'

# 统计页迁移事件
bpftrace -e 't:migrate:mm_migrate_pages { @ = count(); }'

# 统计页压缩事件（带时间戳）
bpftrace -e 't:compaction:mm_compaction_begin { time(); }'
```

技巧点：

- **`software:page-fault:1`**：软件事件，冒号后的数字是采样间隔 — `:1` 表示每次缺页都统计（最细）；要降开销改大数字
- **`t:vmscan:**`**：跟踪点通配，一次看清内核回收路径的活跃度分布
- 页迁移/页压缩是 NUMA 平衡与碎片整理的信号：频繁迁移意味着内存布局在抖动

### 每条单行的读法（这 7 条各自在"听"什么）

| 单行 | 监听的事件源 | 异常形态 |
|------|-------------|---------|
| brk 按栈计数 | 堆顶调整 | 稳态进程出现新栈 = 有代码还在长堆 |
| page-fault 按进程 | 缺页（含次级） | 稳态后不归零/单调上涨 |
| page-fault 按用户栈 | 同上+归因 | 哪条代码路径在触发缺页 |
| vmscan 通配 | 回收子系统活跃度 | kswapd 病理性活跃 = 内存压力 |
| hugepage_madvise | 显式巨页申请 | 频繁申请但 hfaults 不涨 = 池不够在退化 |
| mm_migrate_pages | 页迁移 | NUMA 策略在搬页（远端访问+缓存失效） |
| mm_compaction_begin | 碎片整理 | 突增 = 巨页/高阶分配在触发整理（停顿源） |

前 5 条开销趋零可常驻；后 2 条在迁移/整理风暴时有放大效应（事件本身就说明系统在挣扎）。

## HFT 关联

- `software:page-fault:1 { @[comm] = count(); }` 一行即可常驻监控缺页：稳态交易进程应趋近 0，突增即告警
- `t:migrate:mm_migrate_pages` 在 NUMA 交易机上监控页跨节点迁移（迁移 = 远端内存访问 + 缓存失效）
- `t:compaction:*` 突增说明巨页分配在触发碎片整理 — 检查启动期是否预留够了 hugetlb 池

## 常见陷阱

1. **software:page-fault 不写间隔** — 默认采样间隔粗（可能按 100 次采样一次），精确统计要显式 `:1`
2. **把缺页计数当异常** — 启动期大量缺页正常；关键是稳态后是否归零、趋势是否单调上涨（泄漏信号）
