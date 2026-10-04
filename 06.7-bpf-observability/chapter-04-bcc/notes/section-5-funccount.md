# 4.5 funccount

> 库本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.5 节。多用途工具之一：**事件计数器**

## 内容详解

`funccount(8)`：对匹配模式的事件**计数**并定时打印——回答"**谁被调用了多少次？**"

### 书中 5 个示例

```bash
funccount tcp_drop                 # 1. 内核函数 tcp_drop 被调频率
funccount 'vfs_*'                  # 2. 通配符：所有 vfs_ 前缀函数
funccount -i 1 pthread_mutex_lock  # 3. 每秒打印一次（默认每秒，可 -i 10 改间隔）
funccount 'c:strlen'               # 4. 库函数（libc）
funccount 't:syscalls:sys_enter_*' # 5. tracepoint：统计全部 syscall 入口
```

### 事件名语法（5 种形式）

| 语法 | 含义 |
|------|------|
| `name` | 内核函数（kprobe） |
| `lib:name` | 用户态库函数，如 `c:strlen`（uprobes） |
| `/path:name` | 指定二进制路径的函数，如 `/usr/local/bin/app:main` |
| `t:system:name` | tracepoint |
| `u:lib:probe` | USDT 探针，如 `u:node:http__server__request` |

这套语法值得单独记住：**它是 BCC 多用途工具族的公共事件寻址语言**（funccount/stackcount/trace/argdist 通用），学会一次到处用。bpftrace 的探针类型（`kprobe:`/`uprobe:`/`t:`/`usdt:`）是同一套概念的不同拼写。

### 其他

- `-i N`：输出间隔（秒）；
- 支持**单行程序**快速验证；
- `-h` 显示用法；
- 通配符会**为每个匹配函数各建一个 BPF 程序**——匹配过多函数时加载变慢、开销叠加。

### funccount 的读法：变化率比绝对值重要

单次输出只是快照，真正的信息在**时间序列的跳变**里：

```text
-i 1 观察 futex 入口计数:
  12:00:01  t:syscalls:sys_enter_futex     18234
  12:00:02  t:syscalls:sys_enter_futex     19011     ← 基线
  12:00:03  t:syscalls:sys_enter_futex     18455
  12:00:04  t:syscalls:sys_enter_futex    1477282    ← 跳变 80 倍：锁竞争爆发/自旋
  12:00:05  t:syscalls:sys_enter_futex    1399203
```

纪律：**先取基线（3–5 个周期）再对照异常时段**——绝对值没有参照系（18234/s 正常吗？取决于负载），跳变倍数才是自带的参照。

## HFT 关联

- 第一步永远是**估频次**：怀疑某路径慢/被频繁触发，先 `funccount -i 1 <func>` 看 QPS 量级，再决定用 argdist（分布）还是 trace（细节）。
- 例：`funccount -i 1 't:syscalls:sys_enter_futex'` 一眼看出锁系统调用是否异常放大。
- 交易时段对比基准：开盘前后各抓一轮 funccount（与 3.5 的清单存档习惯同构），倍数跳变的函数就是下钻候选——这是"无目标巡检"的安全版（先有基线，再看变化，不算盲巡）。

## 陷阱

- ⚠️ 书中实测：跟踪 `malloc`/`free` 这类超高频函数，funccount 也能带来 **~30% 开销**（第 3 章数据）——高频目标要评估后短窗口使用。
- ⚠️ 通配符 `vfs_*` 在大内核上可能匹配几百个函数；BCC 逐个建立 kprobe，启动慢且达上限（`sysctl kernel.kptr_restrict`、probe 数量限制）时失败。
- ⚠️ funccount 只数"入口次数"，不区分成功/失败、不看参数——看到计数高不等于"有效工作量大"（可能全是快速失败重试），需要区分时上 argdist。

<details>
<summary>自测题</summary>

1. 统计 libc 中 strlen 的调用频率怎么写？
   <details><summary>答案</summary>`funccount -i 1 'c:strlen'`。</details>

2. `t:syscalls:sys_enter_read` 前缀 `t:` 表示什么？
   <details><summary>答案</summary>tracepoint（system 名 `syscalls`、事件名 `sys_enter_read`）。</details>

3. 为什么 funccount 挂在 malloc 上开销显著？
   <details><summary>答案</summary>malloc 极高频（每秒百万级），每个 kprobe 命中都走 trap，实测约 30% 额外开销。</details>

4. 为什么读 funccount 输出要"先基线后对比"，而不是看绝对值？
   <details><summary>答案</summary>绝对值没有参照系（多高算高取决于负载与机器）；基线给出正常水位后，跳变倍数自带参照——80 倍跳变无论基线是多少都指向异常。</details>

5. BCC 多用途工具的"公共事件寻址语言"有哪几种形式？
   <details><summary>答案</summary>裸名（内核函数）、lib:name（库函数）、/path:name（指定二进制）、t:system:name（tracepoint）、u:lib:probe（USDT）——funccount/stackcount/trace/argdist 通用。</details>
</details>
