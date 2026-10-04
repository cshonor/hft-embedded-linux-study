# 5. BPF 工具：blkthrot 与 overlayfs（15.3.3–15.3.4）

> 底本：《BPF之巅》第 15 章 容器，15.3.3–15.3.4 节（印刷 p714–716）

## 15.3.3 blkthrot

作者 2019-05-06 开发。统计 cgroup **blkio 控制器按硬限制（throttle）限制块 I/O 的次数**：

```
# blkthrot.bt
@not_throttled[1]: 506
@throttled[1]: 31        ← blk cgroup ID=1 被限了 31 次
```

- 实现：**kprobe/kretprobe `blk_throtl_bio()`**，返回值非 0 = 被限
- 块 I/O 相对低频 → 开销很小
- 另一方法：在块操作完成时检查 bio 结构体的 **BIO_THROTTLED 标志**

源代码（提取 cgroup ID 的模板）：

```bash
#include <linux/cgroup-defs.h>
#include <linux/blk-cgroup.h>

kprobe:blk_throtl_bio
{ @blkg[tid] = arg1; }

kretprobe:blk_throtl_bio
/@blkg[tid]/
{
    $blkg = (struct blkcg_gq *)@blkg[tid];
    if (retval) {
        @throttled[$blkg->blkcg->css.id] = count();
    } else {
        @not_throttled[$blkg->blkcg->css.id] = count();
    }
    delete(@blkg[tid]);
}
```

**cgroup ID 在 cgroup_subsys_state（css）结构体中**——这里是 blkcg 结构体的 css 字段；其他子系统 cgroup ID 同法提取。

## 15.3.4 overlayfs

作者同事 Jason Koch 2019-03-18 为处理容器性能问题开发。跟踪 **Overlay 文件系统**（容器镜像常用）的**读写延迟**直方图：

```
# overlayfs.bt 4026532311          ← 参数 = PID 命名空间 ID
21:21:06
@write_latency_us:
[128, 256]: 11

@read_latency_us:
[8, 16]:   ...
[16, 32]:  115     ← 读通常 16~64us
[32, 64]:  123
```

- 实现：kprobe/kretprobe Overlay 的 file_operations 读写函数 **ovl_read_iter / ovl_write_iter**（**Linux 4.19 加入**）
- **过滤器用 pidns**：`((struct task_struct *)curtask)->nsproxy->pid_ns_for_children->ns.inum == $1`
- 开销与调用频率成比例，多数负载可忽略

### 配套 shell 脚本（容器 ID → pidns 的桥）

```bash
#!/bin/bash
# overlayfs.sh <docker容器ID>
PID=$(docker inspect -f '{{.State.Pid}}' $1)
bpftrace ./overlayfs.bt $NSID
```

**为什么必须有这一步**（呼应 15.1.2）：内核没有容器 ID，只能从**用户态**把容器 ID 换算成内核可匹配的 PID 命名空间。可按自己的容器技术调整脚本。

## HFT 关联

- blkthrot 对应"容器磁盘配额打满"：日志/落盘服务被 blkio 限流时，上层只见 I/O 变慢——`@throttled` 计数一翻就知道是配额问题不是盘的问题
- overlayfs 读延迟 16~64us：容器内策略代码读配置/证书走 overlay 层，多层镜像的读放大要心里有数；延迟敏感路径应把数据拷到 tmpfs/挂载卷
- "用户态脚本做容器 ID→内核键换算"是自研容器观测管道的必备胶水层

<details>
<summary>自测题</summary>

1. blkthrot 跟踪哪个内核函数？如何判断被限流？
   <details><summary>答</summary>blk_throtl_bio 的 kprobe+kretprobe；retval 非 0 即被限流（也可事后查 bio 的 BIO_THROTTLED 标志）。</details>

2. cgroup ID 藏在哪个结构体？
   <details><summary>答</summary>cgroup_subsys_state（css）结构体中；blkio 场景经 blkcg_gq→blkcg→css.id 提取。</details>

3. overlayfs 工具为什么需要 shell 脚本包装？
   <details><summary>答</summary>内核没有容器 ID，脚本用 docker inspect 把容器 ID 换算成 PID 命名空间 ID（bpftrace 过滤键）。</details>

4. ovl_read_iter/ovl_write_iter 是哪个内核版本加入的？
   <details><summary>答</summary>Linux 4.19。</details>
</details>
