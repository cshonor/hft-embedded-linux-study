# 5. 三大变量类型

### 内置变量 (Built-in)

探针触发时 **只读** 的上下文 — 无需声明：

| 变量 | 含义 |
|------|------|
| `pid` | 进程 ID |
| `tid` | 线程 ID |
| `comm` | 进程名（16 字符截断） |
| `uid` / `gid` | 用户/组 |
| `nsecs` | 纳秒时间戳（单调时钟） |
| `elapsed` | 距脚本启动纳秒数 |
| `cpu` | 当前 CPU 编号 |
| `retval` | **kretprobe/uretprobe** 返回值 |
| `arg0`…`arg5` | 探针参数（位置因探针类型而异） |
| `kstack` | 内核栈（配合 `print(kstack)` 或作 map 键） |
| `ustack` | 用户栈 |

### 临时变量 (Scratch) — `$` 前缀

当前动作块内 **局部** 计算：

```bash
bpftrace -e '
kprobe:tcp_sendmsg
{
    $size = arg2;
    @total = sum($size);
}
'
```

### 映射表 (Maps) — `@` 前缀

**跨事件存储与关联的核心** — 底层即 BPF Map：

```bash
# 按线程记录开始时间
@start[tid] = nsecs;

# 按 comm 计数
@[comm] = count();

# 全局单值
@bytes = sum(arg2);
```

| 键类型 | 用途 |
|--------|------|
| 标量 `@x` | 全局计数/求和 |
| `@x[key]` | 按 PID、comm、栈 ID 等维度聚合 |
| 嵌套 `@x[a,b]` | 二维统计 |

**程序结束** 时，bpftrace **默认自动打印** 所有 `@` map 内容（可用 `print()` 自定义时机）。


### 常见陷阱

1. **混淆 $ 临时变量和 @ Map 变量** — $ 变量只在当前 probe 块内有效（不可跨事件），@ 变量持久存储在 Map 中（跨事件共享）；用 $ 做 Map key 不会持久化
2. **以为内置变量可以修改** — 内置变量（pid/comm/nsecs 等）是只读上下文，不能赋值；尝试 `pid = 0` 会报错
3. **忽视 Map 的自动打印行为** — 脚本退出时所有 @ Map 自动打印；如果不想要默认输出，在 END 中 clear() 或只用 print() 控制输出

<details>
<summary>📝 自测题（点击展开）</summary>

1. **bpftrace 的三大变量类型分别是什么？各有什么特点？**

   <details>
   <summary>参考答案</summary>

   (1) 内置变量（无前缀）：只读上下文，如 pid/tid/comm/nsecs/cpu/arg0-5/kstack/ustack，探针触发时自动填充。(2) 临时变量（$ 前缀）：当前 probe 块内局部，如 `$start = nsecs;`，跨事件无效。(3) Map 变量（@ 前缀）：跨事件持久存储，如 `@count[comm] = count()`，脚本退出时自动打印。

   </details>

2. **如何用 Map 变量实现「记录函数入口时间、出口算延迟」？**

   <details>
   <summary>参考答案</summary>

   ```bpftrace
kprobe:do_sys_open { @start[tid] = nsecs; }
kretprobe:do_sys_open /@start[tid]/ { @latency = hist(nsecs - @start[tid]); delete(@start[tid]); }
```
用 tid 作 key 存入口时间，retprobe 时取出算差值，用 hist() 画延迟直方图，delete 清理避免 Map 膨胀。

   </details>

3. **Map 变量的自动打印行为是什么？如何控制？**

   <details>
   <summary>参考答案</summary>

   脚本退出（Ctrl-C）时，bpftrace 自动遍历所有 @ Map 并打印内容。控制方式：(1) END 块中 `print(@map)` 自定义输出；(2) `clear(@map)` 清空不打印；(3) `delete(@map[key])` 删除特定条目；(4) 用 interval 定期 `print(); clear()` 实现滚动窗口。

   </details>

</details>

---
