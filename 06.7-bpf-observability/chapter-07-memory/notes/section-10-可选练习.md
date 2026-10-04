# 7.5 可选练习

> 底本：《BPF之巅》第 7 章 内存，7.5 节（印刷 p289）。10 题，未特别说明均可用 bpftrace/BCC 实现 — 括号内为学习提示。

## 观察题（1–4）

1. 生产/服务器上跑 `vmscan` 10 分钟；若 **D-RECLAIMms** 有消耗，接着跑 `drsnoop` 测量事件细节。（体会两级递进：总量→逐事件）
2. 修改 vmscan，每 20 行打印一次表头（interval 计数器 + 条件 printf，练习 bpftrace 状态管理）。
3. 应用程序启动时用 `faults` 统计缺页调用栈（需支持栈+符号的应用；参见第 13/18 章）。
4. 从第 3 题结果生成**缺页错误火焰图**（折叠栈 → flamegraph.pl）。

## 开发题（5–8）

5. 用 brk(2) 和 mmap(2) 跟踪进程**虚拟内存增长**的工具（两个跟踪点 + 增量计算）。
6. 打印 brk(2) 导致的**堆扩展大小**（跟踪点/kprobe/libc USDT 皆可；对比新旧 brk 值）。
7. 显示**页压缩耗时**：compaction:mm_compaction_begin/end 双跟踪点，逐事件时间 + 直方图（双探针计时模板的又一次练习）。
8. 显示 **slab 收缩耗时，按 slab 名称/收缩函数名**分组（kprobe shrinker 函数，func 做键）。

## 测量题（9）

9. 测试环境用 `memleak` 找某软件的长期内存占用，**同时测量 memleak 自身的性能影响**（对比有无跟踪的运行时间 — 建立"跟踪开销"的量感）。

## 开放难题（10）

10. （未解决）调查频繁换页：展示**每个内存页在换页设备上的存活时间**直方图（需测量换出↔换入配对时间，页粒度关联难度大）。

### 开发题参考骨架（自测后再看）

题 5+6 合并（虚拟内存增长监控——brk/mmap 两跟踪点）：

```awk
tracepoint:syscalls:sys_enter_brk /pid == $1/ {
    printf("%s brk(0x%lx)\n", comm, args->brk);
}
tracepoint:syscalls:sys_enter_mmap /pid == $1/ {
    printf("%s mmap len=%lu\n", comm, args->len);
}
// 增量计算：sys_exit_brk 的 ret 是新 break——exit.ret - entry.brk 即本次扩展字节数
```

题 7（compaction 耗时——5.17 双探针计时模板的又一遍）：

```awk
tracepoint:compaction:mm_compaction_begin { @start[tid] = nsecs; }
tracepoint:compaction:mm_compaction_end /@start[tid]/ {
    @ns = hist(nsecs - @start[tid]);   // 模板三件套：存→过滤求差→delete
    delete(@start[tid]);
}
```

题 8 的结构提示：kprobe `shrink_slab` / 各 `shrinker` 回调，键取 `func`（bpftrace 内置变量，探针对应的函数名）——`@[func] = hist(...)`，连"哪个收缩器最慢"一起回答。

## HFT 建议优先级

- **必做**：1（vmscan→drsnoop 递进是无 swap 交易机的核心技能）、3+4（缺页火焰图定位 RSS 增长）
- **选做**：6（堆扩展大小——brkstack 的进阶）、9（建立 BPF 开销量感，知道何时该退回采样）
- 巨页用户加练：结合 hfaults 验证练习 3 的应用是否真用了巨页

## 常见陷阱

1. 练习 1 在健康机器上 D-RECLAIM 恒为 0 — 需要人为制造内存压力（stress-ng --vm）才能看到数据
2. 练习 7/8 是双探针计时模板的变体 — 忘加 `/@start[tid]/` 过滤会混入未配对事件
