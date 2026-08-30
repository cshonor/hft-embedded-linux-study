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

### 其他

- `-i N`：输出间隔（秒）；
- 支持**单行程序**快速验证；
- `-h` 显示用法；
- 通配符会**为每个匹配函数各建一个 BPF 程序**——匹配过多函数时加载变慢、开销叠加。

## HFT 关联

- 第一步永远是**估频次**：怀疑某路径慢/被频繁触发，先 `funccount -i 1 <func>` 看 QPS 量级，再决定用 argdist（分布）还是 trace（细节）。
- 例：`funccount -i 1 't:syscalls:sys_enter_futex'` 一眼看出锁系统调用是否异常放大。

## 陷阱

- ⚠️ 书中实测：跟踪 `malloc`/`free` 这类超高频函数，funccount 也能带来 **~30% 开销**（第 3 章数据）——高频目标要评估后短窗口使用。
- ⚠️ 通配符 `vfs_*` 在大内核上可能匹配几百个函数；BCC 逐个建立 kprobe，启动慢且达上限（`sysctl kernel.kptr_restrict`、probe 数量限制）时失败。

<details>
<summary>自测题</summary>

1. 统计 libc 中 strlen 的调用频率怎么写？
   <details><summary>答案</summary>`funccount -i 1 'c:strlen'`。</details>

2. `t:syscalls:sys_enter_read` 前缀 `t:` 表示什么？
   <details><summary>答案</summary>tracepoint（system 名 `syscalls`、事件名 `sys_enter_read`）。</details>

3. 为什么 funccount 挂在 malloc 上开销显著？
   <details><summary>答案</summary>malloc 极高频（每秒百万级），每个 kprobe 命中都走 trap，实测约 30% 额外开销。</details>
</details>
