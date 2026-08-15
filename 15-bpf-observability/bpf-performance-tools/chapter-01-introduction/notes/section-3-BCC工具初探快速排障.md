# 3. BCC 工具初探 · 快速排障

### execsnoop — 谁在疯狂拉起进程？

```bash
sudo execsnoop-bpfcc    # 或 execsnoop，视发行版包名
```

**场景：** 后台服务每秒尝试启动却失败 — 传统日志可能无记录，**exec 事件** 在 BPF 里一览无余（父进程、命令行、返回值）。

**HFT：** 异常 watchdog、僵尸 helper、错误 cron — 排查 **非策略进程** 干扰 CPU cache / 磁盘。

### biolatency — 磁盘 I/O 延迟分布

```bash
sudo biolatency-bpfcc -F -m 5 10
```

**输出：** 块 I/O **延迟直方图**（毫秒桶），10 秒窗口。

**场景：** 「磁盘慢」不能只看平均 — **长尾桶**（如 >32 ms）暴露 journal、日志盘、误配 NFS 等问题。

**HFT：** 共置裸机若出现块设备延迟，常是 **日志/监控/agent** — 与 [SysPerf Ch 9 磁盘](../../../../14-systems-performance/chapter-09-disks/) 的 USE + 直方图方法论一致。


### 常见陷阱

1. **一上来就用最重的工具** — 新手常直接 `perf record` 全系统采样，但快速排障应先用轻量工具（execsnoop、opensnoop）定位现象，再逐步钻取
2. **忽视 BCC 工具的命名规律** — BCC 工具名即问题描述：`biosnoop` = bio + snoop、`runqlat` = run queue + latency；理解命名就能快速选对工具
3. **在错误的层级使用工具** — 比如用 `biolatency` 排查网络延迟问题——BCC 工具按资源域分类（CPU/内存/磁盘/网络），跨域使用会浪费时间

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BCC 快速排障的推荐顺序是什么？**

   <details>
   <summary>参考答案</summary>

   先轻后重：(1) execsnoop/opensnoop 看进程和文件活动（低频、低开销）；(2) biolatency/runqlat 看资源延迟分布；(3) profile 做 CPU 采样定位热点。从「谁在做什么」到「花了多久」再到「CPU 在哪」。

   </details>

2. **如何通过工具名快速判断用途？**

   <details>
   <summary>参考答案</summary>

   BCC 工具名 = 资源 + 动作：bio = block I/O、runq = run queue、off = off-CPU、snoop = 逐事件追踪、lat = 延迟直方图。例如 `offcputime` = off-CPU 时间统计，`biolatency` = block I/O 延迟分布。

   </details>

3. **在 HFT 排障中，为什么不应一上来就全系统 perf record？**

   <details>
   <summary>参考答案</summary>

   全系统 perf record 开销大、数据多、分析慢。HFT 延迟尖刺往往是特定路径的短事件，应先用低开销工具（execsnoop 看进程切换、runqlat 看排队延迟）缩小范围，再对疑似路径做精准 BPF 追踪。

   </details>

</details>

---
