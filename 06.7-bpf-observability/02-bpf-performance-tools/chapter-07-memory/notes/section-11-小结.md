# 7.6 小结

> 底本：《BPF之巅》第 7 章 内存，7.6 节（印刷 p290）。

## 原书小结

- 进程如何使用**虚拟内存与物理内存**（分配器分层、缺页机制、kswapd/直接回收/OOM/换页）
- **传统工具**的内存分析方法：不同类型的用量分析（容量视角）
- **BPF 工具**测量的内存事件（频率与耗时）：
  - OOM Killer（oomkill）
  - 用户态内存分配（memleak）
  - 内存映射（mmapsnoop）
  - 缺页错误（faults / ffaults / hfaults）
  - vmscan（vmscan）与直接回收（drsnoop）
  - 页换入（swapin）

## 问题 × 工具速查

| 问题 | 工具 | 开销 |
|------|------|------|
| OOM 杀了谁、谁触发 | oomkill | ≈0（可常驻） |
| 内存泄漏在哪条路径 | memleak（-S 采样） | 高！调试用 |
| 谁映射了什么文件 | mmapsnoop | ≈0 |
| 堆因谁而涨 | brkstack | ≈0 |
| RSS 因哪条路径增长 | faults（缺页栈/火焰图） | 低 |
| 缺页在读哪些文件 | ffaults | 缺页高频时有影响 |
| 巨页用上了吗 | hfaults | ≈0 |
| 回收耗时、谁在忙 | vmscan | 低（跟踪点聚合） |
| 直接回收卡了谁多久 | drsnoop | ≈0 |
| 换入卡了谁 | swapin | ≈0 |
| 哪些路径在分配（粗粒度） | profile（Ch6） | ≈0 |

## 开销纪律（与 Ch6 一致）

- 低频事件（OOM/mmap/brk/换页/vmscan）→ 跟踪开销可忽略，可常驻
- 高频事件（malloc 每秒百万次）→ memleak 是调试工具；分配路径画像用 profile 采样替代

### 内存排障三段式（本章工具的战术编队）

```
第一段 · 容量与出口（常驻零开销）
  oomkill + vmscan + swapin —— 有没有到危险的边缘？
   └─ 异常 → 第二段
第二段 · 路径画像（低开销短期）
  brkstack / faults / hfaults —— 增长和缺页从哪来？
   └─ 需要精确到分配 → 第三段
第三段 · 精确记账（测试环境）
  memleak -S + 读代码定性 —— 悬账是否真是泄漏？
```

每段只在前一段出异常时进入——这套递进本身就是开销纪律的执行形态。

## HFT 收尾 Checklist

- [ ] oomkill 常驻 + 关键进程 oom_score_adj 保护（无 swap 机器唯一故障出口）
- [ ] vmscan 常驻盯 D-RECLAIM（>0 告警）→ drsnoop 定位受害者
- [ ] swapin/si/so 恒为 0（无 swap 或无换页活动）
- [ ] 策略启动后 `software:page-fault:1` 计数趋零（prefault + mlockall + 巨页，hfaults 验证）
- [ ] memleak 只在测试环境 + -S 采样使用
- [ ] brkstack/faults 解释任何 RSS 增长事件

## 相关章节

- 缺页与文件缓存深入（mmapfiles/fimapfaults/cachestat）：[chapter-08 文件系统](../../chapter-08-file-systems/)
- 内核内存（kmem/slabratetop/numamove）：[chapter-14 内核](../../chapter-14-kernel/)
- MM 理论：[06-linux-mm](../../../../06-linux-mm/) · [06.5-modern-mm](../../../../06.5-modern-mm/)
- SysPerf 对照：[06.6-systems-performance/chapter-07-memory](../../../../06.6-systems-performance/)
- 下一章文件系统：[chapter-08-file-systems](../../chapter-08-file-systems/)
