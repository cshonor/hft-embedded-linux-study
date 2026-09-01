# 6. BPF 单行程序与练习（15.4–15.5）

> 底本：《BPF之巅》第 15 章 容器，15.4–15.5 节（印刷 p717–718）

## 15.4 BPF 单行程序（两个）

```bash
# 以 99Hz 的频率对 cgroup ID 进行计数
bpftrace -e 'profile:hz:99 { @[cgroup_id] = count(); }'   # （书中示意：按当前任务的 cgroup 分解采样）

# 跟踪名为 "container1" 的容器（cgroup v2）中打开的文件名
bpftrace -e 'tracepoint:syscalls:sys_enter_openat
    /cgroup == cgroupid("/sys/fs/cgroup/unified/container1")/
    { printf("%s\n", str(args->filename)); }'
```

要点：**cgroupid()** 内置函数把 cgroup v2 路径换算成 ID，再与当前任务的 cgroup 比较——cgroup v2 版的"容器过滤"单行写法。

## 15.5 可选练习（3 题）

1. 修改第 6 章的 runqlat(8)，包含 **UTS 命名空间节点名**（参考 pidnss(8)）
2. 修改第 8 章的 opensnoop(8)，包含 UTS 命名空间节点名
3. 开发一个工具显示由于 **memcg 导致的容器换出**（参考 mem_cgroup_swapout() 内核函数）

（练习可用 bpftrace 或 BCC 完成。）

### 参考骨架

**练习 1/2（同一模式：给既有工具加 nodename 键）**——以 opensnoop 为例，改动只在事件处理块加一行取键：

```bash
bpftrace -e '
#include <linux/nsproxy.h>
#include <linux/utsname.h>
tracepoint:syscalls:sys_enter_open
{
    $uts = ((struct task_struct *)curtask)->nsproxy->uts_ns;
    @[str($uts->name.nodename), comm] = count();   // 或并入原输出列
}'
```

要点：①`#include` 让 BTF 解析结构体链 nsproxy→uts_ns→name；②nodename 是 char 数组要 str() 截断；③pidns 版换 `pid_ns_for_children->ns.inum`（数值键不用 str）。runqlat 同理把 `@start[tid]` 之外多存一份 nodename（或输出时反查）。

**练习 3（memcg 换出计数）**：

```bash
bpftrace -e '
#include <linux/memcontrol.h>
kprobe:mem_cgroup_swapout
{
    $memcg = (struct mem_cgroup *)arg1;    // 签名以目标内核源码为准
    @[ $memcg->css.id ] = count();         // css.id = cgroup ID（blkthrot 同款键）
}'
```

要点：①css.id 提取模板直接复用 15.3.3 blkthrot 的写法；②arg 位置必须先查当前内核的 mem_cgroup_swapout 签名（kprobe 无契约，见 1.7）；③要区分"换出是因为 memcg limit 还是全局内存压力"，加对 `try_to_free_mem_cgroup_pages` 的栈归因（stackcount）更完整。

## HFT 关联

- cgroupid() 过滤单行可作模板：K8s 上按 cgroup（pod）过滤任何系统调用的通用套路，比 pidns 过滤更贴近 K8s 的资源模型（cgroup v2）
- 练习 3 的 mem_cgroup_swapout 值得真做：容器内存 limit 逼近时静默换页，是容器化服务尾延迟劣化的隐形来源

<details>
<summary>自测题</summary>

1. cgroupid() 函数的作用？
   <details><summary>答</summary>把 cgroup v2 路径换算为 cgroup ID，供过滤器中与当前任务的 cgroup 比较。</details>

2. 三道练习分别练什么？
   <details><summary>答</summary>给既有工具加 UTS nodename 键（容器感知改造 ×2）；用 mem_cgroup_swapout 内核函数跟踪 memcg 导致的换出。</details>
</details>
