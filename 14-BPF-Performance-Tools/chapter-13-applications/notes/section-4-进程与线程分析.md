# 4. 进程与线程分析

### `execsnoop`

后台频繁 **短生命周期子进程**（shell 脚本）— 极低效。

→ [Ch 6](../../chapter-06-cpus/) · [Ch 11 安全](../../chapter-11-security/)

```bash
sudo execsnoop-bpfcc
```

### `threadsnoop`

追踪 **`pthread_create()`** — 线程池何时扩容、入口函数。

```bash
sudo threadsnoop-bpfcc -p $(pidof myapp)
```

### `threaded`

采样 **哪些线程在 CPU 上跑** — 验证 **负载均衡**（是否单线程 100%、其余闲置）。

```bash
sudo threaded-bpfcc -p $(pidof myapp) 10
```

**HFT：** 多线程策略若 **`threaded` 显示一核独忙** — 并行度或绑核配置错误。

---
