# 4.13 小结

> 库本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.13 节（印刷 p136）

## 内容详解

本章要点（原书小结）：

1. **BCC 提供 70+ 个 BPF 性能工具**，多数支持命令行参数定制行为；
2. **全部工具带文档**：man 帮助手册 + 示例文件；
3. **大部分是单一用途工具**——专注于把某一事件观测好（UNIX 哲学）；
4. **少数多用途工具**，本章精讲 4 个：
   - `funccount(8)`——对事件**计数**；
   - `stackcount(8)`——对导致事件发生的**调用栈**计数；
   - `trace(8)`——自定义**逐事件打印**；
   - `argdist(8)`——参数/返回值的**频率或直方图**统计；
5. 本章还介绍了 **BCC 调试工具**（printf、调试标志位、--ebpf、bpflist、bpftool、dmesg、reset-trace）；
6. **附录 C** 提供开发新 BCC 工具的参考例子。

## 四大多用途工具 · 一图选型

```
事件频率高？
 ├─ 是 → 只想知道"多少次"     → funccount
 │       想知道"什么值/分布"   → argdist (-C/-H)
 │       想知道"哪条路径"      → stackcount (-f 出火焰图)
 └─ 否（低频）→ 想看每一次细节 → trace（含 r:: 返回值、内核态过滤）
```

### 工具间的接力：一个完整的排障故事

四大工具真正按"接力棒"协作（每棒回答一个问题，并决定下一棒）：

```text
现象: 下单延迟偶发尖刺
 └─① funccount 'r::my_sendmsg (retval!=0)'    "错误次数多吗？" → 每秒 3 次，确实在发生
    └─② argdist -H 'r::my_sendmsg:$latency'   "慢在什么量级？" → 双峰: 50µs 常峰 + 8ms 异常峰
       └─③ stackcount -f 'my_sendmsg (latency>1ms)'  "慢的调用从哪来？" → 火焰图: 都经重试路径
          └─④ trace 'my_sendmsg (latency>1ms)'  "那几笔的细节？" → 参数: 全是对端 X 的连接
结论: 对端 X 偶发不收 → 重试路径 8ms——不是本端问题
```

四问四答：**多少次 → 什么分布 → 哪条路径 → 什么细节**——每一步的输出都是下一步的过滤条件，开销也逐级放大（计数 < 直方图 < 抓栈 < 逐事件）。这个顺序本身就是开销纪律。

## 本章坑点速查

| 坑 | 一句话提醒 |
|----|-----------|
| `trace` 挂高频事件 | 人为性能事故；先 funccount 估频次 |
| funccount 挂 malloc | ~30% 开销实测；短窗口使用 |
| 栈"跳帧" | 内联函数无栈帧，是常态不是 bug |
| `r::` 漏写 | 入口探针拿不到 retval |
| argdist 缺 `-C/-H` | 不输出任何东西 |
| 工具名找不到 | 发行版后缀：`-bpfcc` / `bcc.` 前缀 |
| 内核拒绝加载 | `dmesg` 看验证器/探针错误；`--ebpf` 打出程序源码排查 |
| 老内核崩溃遗留事件源 | 4.17+ 已根治（perf_event_open fd 自动回收） |
| 配对对账误报 | 中途退出的进程由内核批量清理，差值短暂不归零 |
| BCC 能编译的 C 到 libbpf 被拒 | 改写器方言（隐式 probe_read）要手工改显式 |

## HFT 落地建议

1. 巡检工具箱只从 tools/ 挑单用途工具（man 的 OVERHEAD 达标的），四大多用途留给排障；
2. 排障流程固化：funccount 估量 → argdist 看分布 → stackcount 找路径 → trace 抓细节（低频）——四问四答写进 runbook；
3. 自研观测工具用 BCC 开发（参数化、可维护），验证假设用 bpftrace（第 5 章）；
4. 新部署一律走 libbpf-tools/CO-RE 形态（静态二进制 + BTF），BCC 运行时编译只留在开发机。

## 交叉引用

- 前一章：[chapter-03-performance-analysis](../../chapter-03-performance-analysis/README.md)
- 下一章：[chapter-05-bpftrace](../../chapter-05-bpftrace/README.md)（bpftrace 语言专章）
- 全书目录：[BOOK-TOC.md](../../BOOK-TOC.md)

<details>
<summary>自测题</summary>

1. 用一句话说清四大多用途工具各自回答的问题。
   <details><summary>答案</summary>funccount：多少次；stackcount：哪条调用路径；trace：每次事件的细节；argdist：参数/返回值的分布。</details>

2. 开发新 BCC 工具时应参考本书哪个附录？
   <details><summary>答案</summary>附录 C。</details>

3. 复述"四问四答"接力及其开销含义。
   <details><summary>答案</summary>多少次（funccount）→ 什么分布（argdist）→ 哪条路径（stackcount）→ 什么细节（trace）；开销逐级放大，顺序本身是开销纪律——每步输出还是下步过滤条件，量越走越窄。</details>

4. BCC 的 C 与标准 BPF C 的差异来源是什么？迁移成本花在哪？
   <details><summary>答案</summary>改写器方言：BCC 编译前自动把指针解引用重写为 bpf_probe_read()，比标准宽容。迁移 libbpf/CO-RE 时要把隐式转换全部手工改显式——这是迁移的主要工作量。</details>
</details>
