# 7.3 BPF 工具（四）：缺页错误 — faults / ffaults / hfaults

> 底本：《BPF之巅》第 7 章 内存，7.3.6–7.3.7 与 7.3.11 节（印刷 p277–281, 287）。缺页错误**直接导致 RSS 增长**，且相对低频 — 性价比最高的内存跟踪事件。

## 7.3.6 faults — 按调用栈统计缺页

视角独特：截取的不是"触发分配的路径"，而是**首次触碰该内存（触发缺页）的代码路径** — 正是它让 RSS 增长。

BCC/stackcount 实现（exceptions 跟踪点，分用户态/内核态）：

```bash
stackcount -u t:exceptions:page_fault_user    # 用户态缺页 + 用户栈
stackcount    t:exceptions:page_fault_kernel  # 内核态缺页
stackcount -P -u t:exceptions:page_fault_user # 按进程
```

书例（java 启动）：C2 编译线程的 `PhaseIdealLoop::Dominators → memset_avx2_erms` 路径触发了数千次缺页 — JIT 编译过程在工作内存上产生缺页。

bpftrace 版（软件事件，注意采样频率 1 = 全事件）：

```bash
#!/usr/local/bin/bpftrace
software:page-fault:1
{
    @[ustack, comm] = count();
}
```

**缺页错误火焰图**（图 7-5）：栈折叠后进 flamegraph.pl，可直接看到"哪片代码在让内存涨"。Netflix 内部工具 Vector 内置一键生成（第 17 章）。

## 7.3.7 ffaults — 按文件名统计缺页

回答"缺页来自哪些文件"（分析编译/启动场景的 page cache 行为）：

```
# ffaults.bt（软件编译过程中）
@[libc-2.23.so]:              84814   ← 每个短命程序都要触碰 libc
@[libopcodes-2.26.1-system.so]:46369
@[bash]:                      45236
@[ld-2.23.so]:                27558
@[cc1]:                       23083
@[locale-archive]:            21137
@[:                          537925   ← 无文件名 = 匿名内存（堆等）最大头
```

实现（从内核函数参数一路挖到文件名，结构体指针穿行的范例）：

```bash
#!/usr/local/bin/bpftrace
#include <linux/mm.h>
kprobe:handle_mm_fault
{
    $vma = (struct vm_area_struct *)arg0;
    $file = $vma->vm_file->f_path.dentry->d_name.name;
    @[str($file)] = count();
}
```

- `vma→vm_file→f_path.dentry→d_name.name` 链条：缺页所属虚拟区间的映射文件
- 匿名页（堆/栈）无 vm_file → 空键
- ⚠️ 缺页高频时（编译期 >100 万/s）本工具有性能影响，先用 perf/sar 看 fault 频率再上

## 7.3.11 hfaults — 巨页缺页

按进程统计巨页（huge page）缺页 — **验证巨页真的被用上了**：

```bash
# hfaults.bt
@[884, hugemmap]: 9      ← PID 884 的测试程序触发 9 次巨页缺页
```

```bash
#!/usr/local/bin/bpftrace
BEGIN { ... }
kprobe:hugetlb_fault
{
    @[pid, comm] = count();
}
```

可从参数扩展抓取 mm_struct / vm_area_struct 更多细节（ffaults 的取文件名方法可复用）。

## 三工具对比

| | faults | ffaults | hfaults |
|---|--------|---------|---------|
| 维度 | 调用栈（谁触发） | 文件名（哪个文件） | 进程（巨页归属） |
| 探针 | exceptions 跟踪点 / 软件事件 | kprobe:handle_mm_fault | kprobe:hugetlb_fault |
| 回答 | RSS 因哪条代码路径增长 | 缺页在读哪些文件 | 巨页是否启用、归谁 |

## HFT 关联

- 策略启动**prefault 验证**：hfaults 确认巨页池真的挂上（配置错了一行都不缺页）；faults 看启动后还有没有缺页 — 低延迟要求稳态零缺页
- ffaults 审计启动序列：行情回放文件、模型文件映射后首次触碰都在启动期完成，而不是交易时段
- faults 栈直接解释 RSS 增长来源，比 memleak 便宜得多，是"内存为什么涨"的第一工具

## 常见陷阱

1. **faults 高频场景直接跑 bpftrace 版** — 编译/海量短命进程时缺页每秒百万次，先 sar -B 看频率
2. **ffaults 空键当成错误** — 空文件名 = 匿名内存（堆），往往是最大头，正是要看的对象
3. **以为配了巨页就用上了** — THP/mount + 应用 mmap(HUGETLB) 缺一不可，hfaults 为零说明没走巨页路径
4. **缺页栈与分配栈混淆** — faults 显示的是"首触内存"的路径（可能是 memset/写数据），不是 malloc 的路径

<details>
<summary>📝 自测题（点击展开）</summary>

1. **faults 与 memleak 的视角有什么不同？**

   <details>
   <summary>参考答案</summary>

   memleak 跟踪分配事件（malloc 路径，高频高开销），显示"谁申请了未释放"；faults 跟踪缺页事件（首次访问路径，低频低开销），显示"谁首次触碰了内存让 RSS 增长"。RSS 只在缺页时增长，所以 faults 与 RSS 涨落直接对应；分配了不触碰的虚拟内存不占物理内存。
   </details>

2. **ffaults 如何拿到文件名？匿名内存为什么是空？**

   <details>
   <summary>参考答案</summary>

   kprobe:handle_mm_fault 的 arg0 是 vm_area_struct，沿 vm_file→f_path→dentry→d_name.name 取文件名字符串（str() 转 BPF 字符串）。匿名内存（堆、栈、MAP_ANON）没有 backing file，vm_file 为空，落到空键。
   </details>

</details>
