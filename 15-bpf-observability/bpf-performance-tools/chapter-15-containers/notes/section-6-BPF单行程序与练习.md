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
