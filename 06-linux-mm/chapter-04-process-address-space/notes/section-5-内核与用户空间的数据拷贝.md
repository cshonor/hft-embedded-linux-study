# Ch 4 §5 内核与用户空间的数据拷贝（异常表机制）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`include/linux/uaccess.h` :99-:166 `should_fail_usercopy`/`access_ok`、`arch/*/kernel/extable.c`）

---

## 本节讲什么

内核为什么不能 `memcpy` 用户指针、`copy_to_user` 遇到坏指针为什么不 panic——答案都指向一个优雅的老机制：**异常表（exception table）**。这是"内核优雅处理用户态地雷"的教科书案例，也是理解 syscall 延迟构成的必经一站。

---

## 1. 为什么不能裸解引用

用户指针的三宗罪：

| 罪 | 后果 |
|----|------|
| 可能未映射/已换出 | 内核态访问触发 fault——但内核 fault 没有"用户态缺页处理"兜底 |
| 可能只读而你写 | 同上，权限 fault |
| 恶意指针（用户故意传内核地址） | 安全漏洞（绕过地址空间隔离） |

**第 3 条是内核漏洞史的半壁江山。** `access_ok()` 只查范围（用户上限以内），真正的页有效性靠 fault + 修复。

## 2. 异常表：内核里的"出错跳转表"

链接期生成 `__ex_table` 段，每条目 `<fault_指令地址, fixup_地址>`：

```
copy_from_user 内联汇编：
  1:  mov (%rax),%rdx          ← 这条指令可能 fault
      ...
      .section __ex_table,"a"
      .quad 1b, 3f             ← 出错则跳标号 3
      .text
  3:  mov $-EFAULT,%eax        ← fixup：返回错误码
```

fault 流程：

```
内核态访问坏用户指针 → MMU 异常
  → do_page_fault → 查 VMA：这是 **用户地址** 且 fault 指令在内核？
  → search_exception_tables(fault_ip)   ← 二分查 __ex_table
       命中 → 修改返回地址为 fixup → copy_from_user 返回 -EFAULT
       未命中 → 真内核 bug → oops/panic
```

**要点：** 这是 **数据驱动** 的错误处理——每条可能出错的指令自带修复地址，无需运行时注册。`fixup_exception()` 就是查表函数。用户态 fault 走缺页，内核态 fault 先查表——同一个 arch 入口两条分路。

## 3. `copy_from_user`/`copy_to_user` 的完整成本（v6.6）

以 `copy_to_user`（uaccess.h，`_copy_to_user` 概念展开）为例：

| 步骤 | 成本 | 备注 |
|------|------|------|
| `should_fail_usercopy()` | 1 分支 | fault-injection 钩子（调试用） |
| `access_ok(dst, n)` | 1 比较 | 仅范围检查（编译期已知范围可优化掉） |
| rep movsb / SIMD 拷贝 | n/带宽 | memcpy 级 |
| SMEP/SMAP 防护 | 硬件免费 | x86 内核态直接碰用户页 = fault（5.x 起常开） |

| 拷贝大小 | 典型 syscall 内开销（x86_64 现代机） |
|----------|--------------------------------------|
| 8B（get_user 级） | ~几 ns |
| 64B（订单/报价包） | ~10-20ns |
| 4KiB | ~200-400ns |

**HFT 眼里的 copy_to_user：** 12.5 系列（网络栈）反复出现——`send()`/`recv()` 每次都要过这道门。**这就是 io_uring/send_zerocopy 想消灭/摊薄的东西**：小包（<1KiB）拷贝其实比 pin+DMA 便宜（12.5/ch13 结论：小消息 ZC 是负优化——成本大头恰是这里省不下的固定开销）。

## 4. `get_user`/`put_user` 与 `_copy` 变体家族

| API | 用途 |
|-----|------|
| `get_user(x, ptr)` / `put_user` | 1/2/4/8B 标量：编译期选指令，异常表单条目 |
| `copy_from_user`/`copy_to_user` | 块拷贝 |
| `unsafe_get_user(...)` + `user_access_begin/end` | v6.6 风格：`user_access_begin()` 关 SMAP 后 **裸访问**（省每条指令的 stac/clac），结束再开——**临界区内 fault 靠异常表** |
| `__copy_from_user_inatomic` | 不睡眠版（持锁场景，spinlock 里） |
| `copy_from_user_nofault` | 探测式（ptrace/perf 用），fault 静默返回错误 |

**`user_access_begin` 家族是 v6.6 源码的主流写法**：旧式逐指令 SMAP 开关（stac/clac 每条 ~1-2 cycle）在大拷贝循环里累积可观，批量关+裸访问+批量开更划算——**又一个"批量摊薄固定开销"的案例**（与 pcp/slub batch 一脉相承）。

## 5. 谁在读你的异常表

```bash
cat /proc/kallsyms | grep __ex_table
dmesg | grep -i 'fixup'          # oops 报告里会写 exception table 命中与否
```

**编程接口：** 内核模块自己写访问用户内存的汇编/优化路径，可用 `.pushsection __ex_table` 注册 fixup（`asm-extable.h` 宏 `_ASM_EXTABLE_U`）。eBPF 里 `bpf_probe_read_user()` 内部就是 nofault 拷贝——06.7 工具读用户内存不崩的底层。

## 6. HFT / 嵌入式关联

| 主题 | 兑现 |
|------|------|
| syscall 预算 | 64B 消息一次 send 的 copy_to_user ~15ns——在 100ns 级 syscall 总预算里占比可观 |
| 批量接口 | `readv/writev`/io_uring 把 N 次 copy 摊进一次 syscall |
| 用户指针安全 | 自研内核模块：永远 copy_from_user 到内核缓冲再校验，**别信用户指针内容**（长度字段也是谎话来源） |
| 探测用户内存 | bpf_probe_read_user（06.7）背你扛 fault |
| 嵌入式无 SMAP | 老芯片裸解引用"能跑"是幻觉——换新 SoC（有 SMAP/SMEP）立刻炸，写模块时按有防护的规矩写 |

## 7. 衔接

- [§4 缺页](./section-4-异常处理与缺页异常.md)：同源异常的两条分路
- [12.5/ch12 io_uring](../../../12.5-modern-networking/chapter-12-io-uring-net/)：消灭拷贝/syscall 边界的工程
- [12.5/ch13 ZC 决策](../../../12.5-modern-networking/chapter-13-zerocopy-highperf/)：copy vs pin 的盈亏平衡
- [06.7 BPF](../../../06.7-bpf-observability/)：nofault 用户内存读

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：异常表和信号处理器的 fixup 是一回事吗？**
A：不是。信号是 **用户态** 机制（内核递送 SIGSEGV 给进程）；异常表是 **内核态** 机制（fault 指令在内核里执行时查表改返回地址）。一个跨用户边界递送，一个就地自我修复。同名不同物，面试易混。

**Q2：`access_ok` 检查了为什么还会 fault？**
A：access_ok 只验 **地址范围**（< TASK_SIZE，防内核地址冒充），不验 **映射存在**。页在不在、权限对不对，只有真访问时 MMU 才知道——所以 access_ok 是廉价的第 1 关，异常表是兜底的第 2 关。两层缺一不可（只留异常表也行但慢路径多）。

**Q3：SMAP 开着，`user_access_begin` 之后中断来了怎么办？**
A：中断入口自己会 `stac/clac` 管理（或直接在关 SMAP 状态进 handler 的代码路径精心审查过）——v6.6 中断路径不碰用户内存，天然安全。临界区长度受控（一个拷贝函数内），不是长驻状态。**"关防护的窗口要函数级短"** 是安全工程通则。

**Q4：`__copy_from_user_inatomic` 在持 spinlock 时 fault 了怎么收场？**
A：查异常表返回 **部分拷贝字节数**（不是 -EFAULT 重试）——调用方拿到"已拷 N 字节"自行决定重试。inatomic 版不能睡眠（缺页换入需要 IO），所以语义只能是"尽力+报告进度"。设计无锁快路径时同款思想：**快路径只承诺尽力，语义完整化交给慢路径**。

**Q5：为什么 perf/bpftrace 读用户内存要用 `copy_from_user_nofault` 而不是 copy_from_user？**
A：观测工具面对的是 **任意时刻的任意进程** 页表——目标页可能换出/被迁移/pin。普通版会阻塞在缺页换入（观测工具拖慢被观测者）；nofault 版失败即返回 0/错误，**观测的可用性优先于完整性**。丢样本不丢延迟。

</details>

---
