# 4. BPF 工具：runqlat 与 pidnss（15.3.1–15.3.2）

> 底本：《BPF之巅》第 15 章 容器，15.3.1–15.3.2 节（印刷 p710–714）

本章 4 个 BPF 工具（表 15-3）：runqlat、pidnss、blkthrot、overlayfs——**要与前几章的工具配合使用**。

## 15.3.1 runqlat（--pidnss）

第 6 章的 runqlat(8) 增加 **--pidnss 选项**：按 **PID 命名空间**（≈容器）分别输出运行队列延迟直方图，识别某容器的 CPU 饱和：

```
host# runqlat --pidnss -m
pidns = 4026532382
     msecs      : count  distribution
     0 -> 1     : 646
     2 -> 3     :  48
     16 -> 31   : 150      ← 延迟长很多
     32 -> 63   : 134

pidns = 4026532870
     0 -> 1     : 264
     ...
```

命名空间 4026532382 的运行队列等待明显更长。

**为什么不打印容器名**：每种容器技术从 PID 查容器名的方法互不相同。至少可以 root 用 ls 查某 PID 的命名空间：

```
# ls -l /proc/181/ns/pid
lrwxrwxrwx 1 root root 0 May ... /proc/181/ns/pid -> 'pid:[4026531836]'
```

→ PID 181 运行在 PID 命名空间 4026531836 中。

## 15.3.2 pidnss

作者 2019-05-06 开发（灵感来自同事 Sargun Dhillon 的建议）。统计调度器上下文切换时的 **PID 命名空间切换**次数 = **CPU 在容器间来回切换的频率**——确认/排除"多容器争用单个 CPU"：

```
# pidnss.bt
Victim PID namespace switch counts [PIDNS, nodename]:
@[0,]:                                  8130    ← 主机（无容器）
@[4026531836, bgregg-i-03cb3a7e46298b38e]: 28   ← 主机 nodename
@[4026532981, 6280172ea7b9]:            27      ← Docker 容器 ID 作 nodename
```

两个键：**PID 命名空间 ID + nodename**（若存在）。Kubernetes 集群搭建期间的真实输出更热闹——coredns/etcd-operator/cilium 等命名空间切换 35~40312 次，两台 bgregg 节点的切换上千次。

### 源代码（kprobe finish_task_switch）

```bash
#!/usr/local/bin/bpftrace
#include <linux/sched.h>
#include <linux/nsproxy.h>
#include <linux/utsname.h>
#include <linux/pid_namespace.h>

kprobe:finish_task_switch
{
    $curr = (struct task_struct *)curtask;
    $prev_pidns = $prev->nsproxy->pid_ns_for_children->ns.inum;
    $curr_pidns = $curr->nsproxy->pid_ns_for_children->ns.inum;
    if ($prev_pidns != $curr_pidns) {
        @[$prev_pidns, $prev->nsproxy->uts_ns->name.nodename] = count();
    }
}
```

- 上下文切换时 prev/curr 的 pidns 不同 → 计一次"被换出的（victim）命名空间切换"
- **这是提取命名空间标识符的模板**：其他命名空间 ID 同法提取
- 需要更多容器细节（超出内核 ns/cgroup 信息）→ 移植到 BCC，加直接查询 Kubernetes/Docker 的代码
- **开销**：kprobe 上下文切换路径，I/O 繁忙负载下明显

## HFT 关联

- runqlat --pidnss 是 K8s 上"哪个 pod 在排队"的一眼定位工具；配合 `cat cpu.stat` 的 throttled 字段区分"限流"还是"邻居抢核"
- pidnss 回答"两个高频容器是否被调度到同一核互相打断"——对绑核部署（cpuset）的验证极有价值：切换次数应接近 0

<details>
<summary>自测题</summary>

1. runqlat --pidnss 的分解键是什么？为什么不显示容器名？
   <details><summary>答</summary>PID 命名空间 ID；各容器技术从 PID 查容器名的方法不同，内核又没有容器 ID（可用 ls -l /proc/PID/ns/pid 人工对照）。</details>

2. pidnss 统计什么事件？用什么判定？
   <details><summary>答</summary>调度器上下文切换中 prev 与 curr 的 PID 命名空间不同的次数（kprobe finish_task_switch + nsproxy 比对）。</details>

3. pidnss 的输出键为什么带 nodename？
   <details><summary>答</summary>Docker/K8s 通常把 UTS nodename 设为容器名，用它把数字 pidns 翻译成人类可读的容器标识。</details>
</details>
