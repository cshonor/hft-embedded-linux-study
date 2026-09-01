## ④ 目录项缓存 · Dentry Cache · dcache

**路径解析**（`/home/dracula/src/the_sun_sucks.c`）— 字符串遍历 + 查找，**昂贵**。

**dcache** 缓存已解析的 **dentry** → 同路径再次访问 **更快**。

#### dentry 三种状态

| 状态 | 含义 |
|------|------|
| **使用中** | VFS **正在用** |
| **未使用** | 暂不用，但 **留在缓存** 备查 |
| **负缓存（negative）** | 路径 **无效/不存在** — 缓存「没有这文件」→ **快速拒绝** 后续无效 open |

```
第一次 open 不存在文件 ──► 负 dentry 入缓存
第二次同路径 open     ──► 不必再深入 FS 查找
```

#### v6.6 源码里负缓存的精确形态

核对 `include/linux/dcache.h`：负 dentry 不是靠单独标志位，而是 **`d_inode == NULL`**（第 89 行注释原话："Where the name belongs to - NULL is negative"）。判定用 `d_is_negative(dentry)`。这带来一个精妙结论：**「这个路径不存在」本身是一条缓存的正知识**——查找在 dcache 命中负 dentry 时即可返回 ENOENT，无需惊动底层 FS。

| dentry 字段（v6.6） | 作用 |
|--------------------|------|
| `d_flags` + `d_lock` | 状态位（DCACHE_REFERENCED 等）与保护锁 |
| `d_parent` / `d_name` | 挂在路径树的形状（`IS_ROOT(x)` 判根：`x == x->d_parent`） |
| `d_inode` | NULL = 负缓存 |
| `d_lockref` | **锁+引用计数合体**（lockref：一句话的并发优化，见下） |

> `lockref` 是 Ch6「结构自带并发协议」的 VFS 实例：传统写法是 spinlock + atomic_t 分离，lockref 把两者压进一个 64bit 字，**无竞争时用一次原子 cmpxchg 同时完成加锁+加计数**——dcache 是全内核最热的锁点之一，这里省下的缓存行往返是真金白银。

#### RCU-walk：现代路径解析的快车道（LKD 时代没有）

路径解析的性能演进分三档（核对 v6.6 `fs/namei.c`，`path_openat` 首选 `flags | LOOKUP_RCU`）：

| 模式 | 持锁情况 | 何时用/何时退 |
|------|----------|--------------|
| **RCU-walk** | **全程不拿任何 dentry 锁**，靠 RCU 宽限期保证 dentry 不被释放，seqcount 检测中途被改 | 快路径首选；遇到 symlink/权限不可达/并发 rename 即**整个 walk 推倒重来** |
| **ref-walk**（慢速） | 逐级拿引用计数（dget/dput），跨 mount 点要处理 | RCU 失败的退路 |
| **旧式（LKD 描述）** | 每级 dentry 都 `d_lock` | 2.6 时代——书里讲的就是这个 |

> RCU-walk 的设计哲学和 [Ch9 per-VMA lock](../../chapter-09-kernel-sync-intro/notes/section-9.3-并发的原因.md) 同宗：**乐观无锁走到底，发现被改就整趟作废重走**。绝大多数 walk 无人竞争，一次成功；少数冲突方退慢路，全局吞吐大幅提升。

| 观测 | `sar -v` dentry/inode cache — [SysPerf §8.6](../../../06.6-systems-performance/chapter-08-file-systems/notes/section-8.6-观测工具.md)；精确数字看 `/proc/sys/fs/dentry-state`（nr_dentry/nr_unused/…） |

**HFT：** 日志/配置 **冷路径** 才关心 dcache；热路径 **已打开 fd** 或 **`mmap`** 绕过反复路径解析。



<details>
<summary>自测题（点击展开）</summary>

**Q1.** dentry cache 如何加速路径解析？

<details><summary>答案</summary>

解析 `/home/user/file.txt` 需要逐级查找：`/` → `home` → `user` → `file.txt`，每级需要读目录项（磁盘 IO）。dentry cache 缓存已解析的路径组件，下次访问同一路径直接从内存中查找，O(1)。热门路径（如 /proc/cpuinfo）几乎永远在 dcache 中。dcache 还通过 hash table 加速查找。

</details>

**Q2.** 为什么 dentry 不能直接释放？reference count 如何工作？

<details><summary>答案</summary>

dentry 有引用计数：每次路径解析经过 dentry 时 +1，结束时 -1。dentry 还有 dcache LRU：即使引用计数为 0 也不立即释放（保留在 LRU 中），下次访问直接命中。内存紧张时从 LRU 尾部回收。这就是为什么 `ls /` 第二次比第一次快。

</details>

**Q3.** RCU-walk 为什么「失败就整趟推倒重走」而不是从失败点续走？

<details><summary>答案</summary>

因为无锁快车道上**不持有任何可"续"的状态**：没拿引用计数、没持锁，唯一的信任基础是「walk 期间没人改路径树」。seqcount 检测到序列号变化，只能说明**某处变了**，但无法便宜地定位"从哪级开始不可信"——逐级验证的成本等于慢速路径。推倒重来的账是划算的：竞争 rename 是稀有事件，为稀有事件在快路径上逐级留验证点，等于把快路径变成慢路径。这是无锁设计的通用取舍：**快路径赌不冲突，慢路径兜底**——与 per-VMA lock 失败回退全局锁同一模板。

</details>

**Q4.** 负 dentry（negative dentry）在 v6.6 源码里如何表示？它防的是什么攻击/负载？

<details><summary>答案</summary>

`dentry->d_inode == NULL` 即负 dentry（dcache.h 注释原文），`d_is_negative()` 判定。它缓存的是**「此路径不存在」**：后续同路径 open 在 dcache 层直接返回 ENOENT，完全不打扰底层 FS。防的负载：① 高频探测不存在的文件（程序反复按若干候选路径找配置——每次都是全链路 lookup 会打爆 inode_operations->lookup）；② **负缓存也占内存**——历史上有个类 DoS：海量打开随机不存在路径，dcache 被负 dentry 撑爆（现代内核对负 dentry 有专门的上限收缩机制）。对 HFT：配置探测逻辑应在启动时做一次性穷举，别在热循环里反复 open 不存在的路径——即使有负缓存，每次 open 也至少要 RCU-walk 全程 + fd 分配。

</details>

</details>
---
