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

**HFT：** 共置裸机若出现块设备延迟，常是 **日志/监控/agent** — 与 [SysPerf Ch 9 磁盘](../../../02-Systems-Performance-2nd/chapter-09-disks/) 的 USE + 直方图方法论一致。

---
