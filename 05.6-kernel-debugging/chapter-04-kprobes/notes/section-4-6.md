# 4.6 Kprobes 与 eBPF 的关系

> 🔴 精读

## 本节要点

### eBPF 如何使用 Kprobes

```
bpftrace / BCC / libbpf
         ↓
    eBPF 程序 (BPF bytecode)
         ↓
    bpf() 系统调用加载到内核
         ↓
    attach 到 kprobe / kretprobe / tracepoint
         ↓
    kprobe 触发 → 执行 eBPF 程序 → 收集数据
```

### Kprobes vs eBPF 对比

| 特性 | kprobes (内核模块) | eBPF (bpftrace/BCC) |
|------|-------------------|---------------------|
| 编程方式 | C 内核模块 | C (BCC) / AWK-like (bpftrace) |
| 编译 | 需编译 .ko | JIT 编译 |
| 安全性 | 可能导致 panic | 验证器保证安全 |
| 性能 | ~1-5μs/次 | ~50-100ns/次 |
| 数据处理 | 回调中处理 | map 聚合 + 用户态读取 |
| 持久性 | 模块加载即生效 | 程序终止即清理 |

### bpftrace 示例

```bash
# 等价于 kretprobe 测量 schedule() 耗时
sudo bpftrace -e '
kretprobe:schedule {
    @sched_ns[pid] = nsecs;
}
kretprobe:schedule /@sched_ns[pid]/ {
    $dur = nsecs - @sched_ns[pid];
    @sched_us = hist($dur / 1000);
    delete(@sched_ns[pid]);
}'

# 追踪 open 系统调用（等价于 kprobe_events）
sudo bpftrace -e '
kprobe:do_sys_openat2 {
    printf("%s opened %s\n", comm, str(arg2));
}'

# 统计函数调用次数
sudo bpftrace -e 'kprobe:__kmalloc { @[comm] = count(); }'
```

### 何时用 Kprobes vs eBPF

| 场景 | 推荐 |
|------|------|
| 快速临时探查 | bpftrace (eBPF) |
| 需要修改内核行为 | kprobes (内核模块) |
| 需要复杂数据处理 | eBPF (map 聚合) |
| 内核版本 < 5.x | kprobes (eBPF 功能不全) |
| HFT 生产环境 | eBPF (更低开销) |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 eBPF 比 kprobes 内核模块更安全？

> eBPF 程序在加载前经过验证器 (verifier) 检查：确保不会越界访问内存、不会无限循环、不会持有锁过久。kprobes 内核模块没有验证，代码 bug 会导致内核 panic。eBPF 程序出错最多是探针失效，不会崩溃内核。

**Q2:** eBPF 的性能为什么比 kprobes 内核模块好？

> eBPF 程序经 JIT 编译为原生指令，在 kprobe 回调中直接执行，无需保存/恢复完整寄存器上下文（验证器保证安全）。kprobes 内核模块需要完整的异常处理流程（保存所有寄存器 → 回调 → 单步执行原始指令 → 恢复）。eBPF 开销约 50-100ns vs kprobes 1-5μs。

</details>
