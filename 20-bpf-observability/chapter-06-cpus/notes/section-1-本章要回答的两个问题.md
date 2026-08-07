# 1. 本章要回答的两个问题

| 问题 | 工具族 | 视角 |
|------|--------|------|
| **CPU 在忙什么？** | `profile`、火焰图、`cpudist`、`syscount` | **On-CPU** — 在核上执行什么代码 |
| **线程为什么得不到 CPU？** | `runqlat`、`runqslower`、`runqlen` | **调度饱和度** — 排队多久才跑上核 |
| **线程不跑时在等什么？** | `offcputime` | **Off-CPU** — 睡眠/阻塞栈与等待时间 |

```
         On-CPU                    Off-CPU
    profile / cpudist          offcputime
    火焰图 / llcstat              |
         \                       /
          \   runqlat（就绪→上核）/
           ----------------------
              调度器 / 运行队列
```


### 常见陷阱

1. **只关注 CPU 利用率忽视饱和度** — 利用率高不一定有问题（可能是计算密集型正常负载），饱和度（排队等待）才是延迟的根因；HFT 更关心饱和度
2. **混淆 on-CPU 和 off-CPU 分析** — on-CPU 分析看「CPU 在执行什么」（热点），off-CPU 分析看「线程为何不在 CPU 上」（等待原因）；两者互补
3. **忽视 CPU 亲和性对 HFT 的影响** — HFT 关键线程应绑定到独立核（isolcpus+taskset），避免迁移和上下文切换开销；不设亲和性会导致 cache miss 和调度抖动

<details>
<summary>📝 自测题（点击展开）</summary>

1. **Ch6 CPU 章节要回答的两个核心问题是什么？**

   <details>
   <summary>参考答案</summary>

   (1) CPU 时间花在哪里？（on-CPU 分析——用 profile 采样、火焰图定位热点）；(2) CPU 时间没花在哪里、为什么？（off-CPU 分析——用 offcputime 看等待原因、runqlat 看排队延迟）。两个问题分别对应「利用率」和「饱和度」。

   </details>

2. **CPU 利用率和饱和度有什么区别？HFT 更关心哪个？**

   <details>
   <summary>参考答案</summary>

   利用率（utilization）：CPU 在执行任务的比例——高利用率不一定是问题（如计算密集型任务）。饱和度（saturation）：任务排队等待 CPU 的程度——饱和度高意味着延迟增加。HFT 更关心饱和度，因为即使利用率不高，偶尔的排队等待也会造成微秒级延迟尖刺。

   </details>

3. **on-CPU 和 off-CPU 分析分别解决什么问题？**

   <details>
   <summary>参考答案</summary>

   on-CPU 分析：线程在 CPU 上执行时，时间花在哪些函数（`profile`/`stackcount`）——定位 CPU 热点。off-CPU 分析：线程不在 CPU 上执行时，在等待什么（`offcputime`）——定位阻塞原因（IO、锁、调度排队）。HFT 延迟分析需要两者结合：on-CPU 看计算耗时，off-CPU 看等待耗时。

   </details>

</details>

---
