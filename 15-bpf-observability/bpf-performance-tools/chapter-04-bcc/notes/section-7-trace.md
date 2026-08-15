# 4.7 trace

> 库本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.7 节。多用途工具之三：**自定义逐事件打印**

## 内容详解

`trace(8)`：BCC 版的 `strace + printf`——对每次命中事件打印自定义格式的行。**多用途工具中最强大也最危险**（逐行打印决定它只能低频使用）。

### probe 语法

```
trace 'probe(signature) (filter) "fmt", args'
```

| 组成 | 示例 | 说明 |
|------|------|------|
| probe | `do_nanosleep`、`r::vfs_read` | `r::` 前缀 = kretprobe（**拿返回值**） |
| filter | `(pid == 1234)` | 内核态过滤 |
| fmt | `"%s"` | 支持 `%s`（字符串）、`%K`（内核符号）、`%U`（用户符号）等 |
| args | `arg2`、`retval`、`comm` | 探针参数 / 返回值 / 进程名 |

### 书中案例

```bash
# 1. 打印 do_nanosleep 的入参（睡眠纳秒数）
trace 'do_nanosleep(u64 *n) "%s", n'   # 配合头文件处理

# 2. 返回值：读操作实际读了多少字节
trace 'r::vfs_read (retval > 0) "read %d bytes", retval'

# 3. 符号格式化：%K 直接把地址渲染成函数名
# 4. -I 包含头文件后可使用结构体：
trace -I 'net/sock.h' \
  'tcp_reset(struct sock *sk) "sk=%K sk->sk_prot->name", sk'
```

### 经典案例：fd 泄露调试（sock_alloc / sock_release）

```bash
trace 'sock_alloc "%d", 1' 'r::sock_release "%d", -1'
```

对 `sock_alloc` 计 +1、`sock_release` 计 -1，两路输出对账——**差值不归零 = socket 未释放（fd/内存泄露）**。展示了 trace 作为"穷人的计数器"的组合用法。

### 其他

- `-I <header>`：包含内核头文件以引用结构体成员；
- `--include`、`-p PID`、`-t` 时间戳等见 `-h`；
- 每次命中打印一行 → **仅适合低频事件或短窗口调试**。

## HFT 关联

- 排障黄金工具：下单路径某个低频函数（如错误分支、超时分支）`trace 'r::my_func (retval != 0) "err=%d pid=%d", retval, pid'`，只在失败时打行，开销可控。
- **严禁**挂到高频热路径（收发包、定时器、malloc）——逐事件走 perf 缓冲区 + 用户态格式化，高频时丢事件且拖慢目标。

## 陷阱

- ⚠️ `r::` 是 kretprobe 的简写（`r:module:function` 中 module 留空）；漏写 `r::` 会在**入口**拿不到 retval。
- ⚠️ 结构体成员访问需要 `-I` 包头文件，否则 Clang 编译报错——报错信息在 stderr，别只看"没输出"。
- ⚠️ filter 写在引号内 `(pid == 1234)`，是**内核态过滤**；写到用户态 `| grep` 就白白付出了所有事件的打印成本。

<details>
<summary>自测题</summary>

1. 怎样只打印 vfs_read 成功且读到的字节数？
   <details><summary>答案</summary>`trace 'r::vfs_read (retval > 0) "read %d bytes", retval'`。</details>

2. `%K` 和 `%U` 的区别？
   <details><summary>答案</summary>`%K` 把内核地址渲染为内核符号；`%U` 渲染为用户态符号。</details>

3. 书中如何用 trace 定位 socket 泄露？
   <details><summary>答案</summary>`sock_alloc` 打 +1、`r::sock_release` 打 -1，对账差值即未释放数量。</details>
</details>
