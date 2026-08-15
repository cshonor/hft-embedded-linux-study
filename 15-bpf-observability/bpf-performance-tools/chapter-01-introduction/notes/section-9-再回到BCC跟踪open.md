# 1.9 再回到 BCC：跟踪 open()

> 底本：《BPF之巅》中文版 1.9 节（PDF p53–54）

## BCC 版 opensnoop(8)

输出列与 bpftrace 版一致（PID/COMM/FD/ERR/PATH），但**命令行参数丰富得多**：

```text
# opensnoop -h
usage: opensnoop [-h] [-T] [-x] [-p PID] [-t TID] [-d DURATION]
                 [-n NAME] [-e] [-f FLAG_FILTER]

  -T, --timestamp       include timestamp on output
  -x, --failed          only show failed opens
  -p PID, --pid PID     trace this PID only
  -t TID, --tid TID     trace this TID only
  -n NAME, --name NAME  only print process names containing this name
  -e, --extended        show extended fields
  -f FLAG_FILTER        filter on flags argument (e.g. O_WRONLY)
```

实战示例——只看失败的 open：

```text
# opensnoop -x
PID    COMM             FD   ERR PATH
991    irqbalance       -1   2   /proc/irq/133/smp_affinity
991    irqbalance       -1   2   /proc/irq/141/smp_affinity
20543  systemd-resolve  -1   2   /run/systemd/netif/links/5
20543  systemd-resolve  -1   2   /run/systemd/netif/links/5
...
```

**不断重复的打开失败**——可能指向程序效率问题或可修复的配置错误（ERR=2 即 ENOENT：文件不存在）。

## BCC vs bpftrace 分工（本章结论性对比）

| | bpftrace 工具 | BCC 工具 |
|---|---|---|
| 风格 | 简单、功能单一、做一件事 | 复杂、多运行模式 |
| 过滤/选项 | 要改源码（如只显示失败 open 得改脚本） | 命令行参数直接支持（`-x`） |
| 定位 | 定制工具、快速问答 | **工作起点**——需要的功能多半已自带 |
| 演进路径 | bpftrace 原型 → 成熟后改写为带参数的 BCC 工具 | BCC 还能组合多事件源：优先 tracepoint、不满足再退 kprobe |

> BCC 编程复杂度高，本书正文聚焦 bpftrace 编程；**附录 C** 提供 BCC 开发快速入门。

---

### HFT 关联

- `-p PID` 按进程过滤是交易机必备：观测窗口只盯策略进程，不把系统其他噪声（监控 agent、日志切割）混进来
- `opensnoop -x` 是发现**配置漂移**的利器：容器化交易组件挂载路径变化后，反复 ENOENT 的重试循环会直接烧 CPU 并拖慢初始化
- bpftrace（原型验证）→ BCC/libbpf（产品化）的演进路径，对应 HFT 观测工具的迭代纪律：先证明指标有用，再工程化常驻

<details>
<summary>📝 自测题（点击展开）</summary>

1. **`opensnoop -x` 输出中 ERR=2 表示什么？这种模式为何值得警惕？**

   <details><summary>参考答案</summary>

   2 = ENOENT（文件不存在）。若同一进程对同一路径反复失败打开，说明配置错误或路径漂移，重试循环本身消耗 CPU，且常意味着程序没拿到它需要的数据（配置、证书、共享文件）。

   </details>

2. **什么信号说明该把 bpftrace 脚本升级为 BCC 工具？**

   <details><summary>参考答案</summary>

   当脚本需要反复使用、需要精细命令行参数（按 PID/TID/时长/失败过滤）、要作为后台进程长跑或与其他事件源组合时——bpftrace 改源码的成本开始超过一次性收益。

   </details>

</details>
