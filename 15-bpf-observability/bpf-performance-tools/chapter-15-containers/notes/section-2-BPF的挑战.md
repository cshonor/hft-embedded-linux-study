# 2. BPF 的挑战（15.1.2 节）

> 底本：《BPF之巅》第 15 章 容器，15.1.2 节（印刷 p704–706）

## 挑战一：BPF 特权

BPF 跟踪需要 **root 特权** → 对多数容器环境意味着**只能在宿主机上执行**，不能在容器内执行。社区正在讨论非特权 BPF 访问问题（参见 11.1.2 节）。

## 挑战二：容器 ID（本章核心问题）

Kubernetes/Docker 的容器 ID 由**用户态软件**管理（`kubectl get pod`、`docker ps`）；内核中一个容器只是**一组 cgroup + 命名空间，没有内核空间的标识符把它们捆绑**（有添加 container ID 的提议但未实现）。

→ 在主机跑 BPF 工具会捕捉**所有容器**的事件，想按容器过滤/分解却没有内核 ID 可用。

### 解决方法：从 nsproxy 提取命名空间标识符

命名空间组合可通过 task_struct 的 **nsproxy 结构体**读取（linux/nsproxy.h：uts_ns / ipc_ns / mnt_ns / net_ns...）：

**方法 1：PID 命名空间 ID**（容器必用 pid 命名空间，可作区分键）：

```bash
#include <linux/sched.h>
$task = (struct task_struct *)curtask;
$pidns = $task->nsproxy->pid_ns_for_children->ns.inum;
```

数值型 pidns 可打印或过滤，匹配 /proc/PID/ns/pid_for_children 符号链接。

**方法 2：UTS nodename = 容器名**（K8s/Docker 运行时通常把 nodename 设为容器名）：

```bash
$nodename = $task->nsproxy->uts_ns->name.nodename;
```

pidnss(8) 工具（15.3.2）就这么做。

**方法 3：网络命名空间 ID**——分析 Kubernetes **pod** 的有用标识符（**pod 内容器共享同一 netns**）。

> 把这些标识符加进前几章的工具即可"容器感知"。**仅在进程上下文插桩时有效**（依赖 curtask）。

## 挑战三：编排

跨多容器主机跑 BPF 工具 ≈ 跨多 VM 云部署的问题。定制方案：**kubectl-trace**——Kubernetes 调度器，在集群中运行 bpftrace 程序；提供 `$container_pid` 变量（容器 root 进程 pid）。例如统计 mypod 的 vfs*() 调用，Ctrl-C 结束；单行或 `-f` 从文件读取。详见第 17 章。

## 挑战四：函数即服务（FaaS）

用户自定义函数由服务商在容器中运行，用户无法 SSH 上去。这类环境**一般不支持最终用户跑 BPF 工具**；非特权 BPF 落地后函数或许能直接调用 BPF，但目前 FaaS 的 BPF 分析**只能在主机上由有权限的用户/接口执行**。

## HFT 关联

- "内核没有容器 ID"是所有容器观测工具的设计前提：自研容器监控选键时，**pidns inum + nodename** 组合是书里给出的标准答案
- K8s pod 共享 netns：网络流量按 pod 归因要用 netns 而不是 pidns

<details>
<summary>自测题</summary>

1. 为什么内核里找不到容器 ID？
   <details><summary>答</summary>容器只是 cgroup+命名空间的组合，由用户态软件（docker/kubectl）管理 ID；内核没有捆绑它们的标识符（container ID 提议未实现）。</details>

2. 从 curtask 提取 PID 命名空间 ID 的 BPF 表达式？
   <details><summary>答</summary>((struct task_struct *)curtask)->nsproxy->pid_ns_for_children->ns.inum</details>

3. 为什么网络命名空间适合标识 Kubernetes pod？
   <details><summary>答</summary>同一 pod 内的多个容器共享同一个网络命名空间。</details>

4. 提取命名空间标识符的限制条件？
   <details><summary>答</summary>仅在进程上下文插桩时有效（依赖 curtask 结构体）；中断上下文等场景不适用。</details>
</details>
