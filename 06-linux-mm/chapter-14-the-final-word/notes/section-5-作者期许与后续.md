# Ch 14 §5 作者期许与后续

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪** · 收尾章

---

## 本节讲什么

原书 §5 是作者期许：读完理论（Ch1–14）再进附录 Code Commentary，并鼓励社区为其他内核子系统写同类「理论 + 代码」著作。

本节把「期许」落成**振鹏自己的后续路线图**——结合本仓库已铺开的模块，给出从「内存管理」到「HFT 系统编程」的完整进阶路径。

---

## 1. 原书期许 → 现代落点

| 原书期许 | 现代落点 |
|----------|----------|
| 读完理论再进 Code Commentary | 本仓库附录 A–M 用 **elixir.bootlin.com + v6.6 缓存源码** 走读 `mm/*.c` |
| 为其他子系统写「理论+代码」著作 | LKD3rd（05-linux-kernel）+ 内核官方 `Documentation/*.rst` 已承接这个角色 |
| 尽量架构无关 | 补一条「架构相关」支线：x86_64 5 级页表 / arm64（07-arm-architecture） |

## 2. 本仓库后续路线（按依赖顺序）

```
                    ┌────────────────────────────┐
 第 0 步（本模块）   │  06-linux-mm：附录 A–M 走读   │  ← 收尾当前主线
                    └────────────────────────────┘
                    ┌────────────────────────────┐
 第 1 步（向下）     │  05-linux-kernel：LKD3rd 对照  │  ← 广度（进程/调度/同步）
                    └────────────────────────────┘
                    ┌────────────────────────────┐
 第 2 步（向上）     │  03-linux-userspace-api：TLPI │  ← 系统调用面（HFT 应用层）
                    └────────────────────────────┘
                    ┌────────────────────────────┐
 第 3 步（架构）     │  07-arm-architecture / 15-arch │  ← 具体 CPU 的 MMU/TLB
                    └────────────────────────────┘
                    ┌────────────────────────────┐
 第 4 步（落地）     │  13-dpdk（大页）+ 14-hft-eng    │  ← 把内存知识变成延迟优势
                    └────────────────────────────┘
```

### 各步与内存管理的衔接点

| 步骤 | 与本书的直接衔接 |
|------|------------------|
| 附录 A–M | 验证 Ch2–13 每个结论在 `mm/*.c` 的真实实现 |
| 05-linux-kernel | `struct mm_struct`/`task_struct`（Ch4）、调度对内存回收的触发（Ch10 kswapd 绑 CPU） |
| 03-linux-userspace-api | `mmap`/`mlock`/`madvise`/`brk` 的系统调用面（Ch4 的入口） |
| 架构线 | 页表结构（Ch3）在 arm64/x86_64 的具体差异、TLB 刷新的架构指令 |
| 落地线 | DPDK 大页 = Ch3 THP + Ch6 高阶分配 + Ch4 mlock 的工程组合 |

## 3. 具体行动清单

```bash
# 1. 附录 A–M 走读（下一步）
#    用 D:\.kernel-ref\ 缓存 + elixir 对照，每篇一个 mm/*.c 文件

# 2. HFT 落地验证三件套
numactl --membind=0 --cpunodebind=0 ./trading_bench   # NUMA 绑定
mlockall(MCL_CURRENT | MCL_FUTURE)                      # 锁 RSS
echo -1000 > /proc/self/oom_score_adj                   # OOM 免疫

# 3. 观测闭环
watch -n1 "grep -E 'pswpin|pswpout|pgalloc|pgmajfault' /proc/vmstat"
```

## 4. 一句话收尾

原书的价值不在「结论」（结论已被 v6.6 推翻一半），而在**「理论怎么落到代码」的方法论**。把这个方法论接上 **v6.6 源码核验 + HFT 落地**，就是本仓库 06-linux-mm 模块做完之后该有的样子。

---

## 衔接

Ch14 收官，意味着 **06-linux-mm 正文（Ch1–14）补强全部完成**。剩下最后一块拼图：附录 A–M 的代码走读（13 个空占位）。

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：本书作者 Mel Gorman 的两条期许是什么？**

① 读者读完理论（Ch1–14）再进附录 Code Commentary，应对 VM 子系统更有信心；② 鼓励社区为其他内核子系统写同类「理论 + 代码」著作。

**Q2：本仓库后续路线的依赖顺序是什么？**

附录 A–M（收尾当前模块）→ 05-linux-kernel（LKD3rd 广度）→ 03-linux-userspace-api（TLPI 系统调用面）→ 架构线（arm64/x86_64）→ 落地线（DPDK 大页 + HFT 工程）。

**Q3：DPDK 大页是本书哪些章节的工程组合？**

Ch3 THP/大页映射 + Ch6 高阶分配（Buddy order>0）+ Ch4 mlock 锁页，三者组合成「固定物理地址 + 稳定 TLB + 免缺页」的 I/O 加速方案。

**Q4：HFT 落地验证的「三件套」是什么？**

NUMA 绑定（numactl --membind/--cpunodebind）、锁 RSS（mlockall）、OOM 免疫（oom_score_adj=-1000）。

**Q5：用一句话概括本书对振鹏的价值？**

不在「结论」（已被 v6.6 推翻一半），而在「理论怎么落到代码」的方法论；把这个方法论接上 v6.6 源码核验 + HFT 落地，才是本模块做完后该有的样子。

</details>
