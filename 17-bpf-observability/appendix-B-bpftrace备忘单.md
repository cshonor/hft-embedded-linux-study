# 附录 B bpftrace备忘单 · bpftrace Cheat Sheet

> **BPF Performance Tools** · Brendan Gregg · **精读**

## 探针类型速查

| 探针 | 语法 | 说明 |
|------|------|------|
| 内核函数入口 | `kprobe:func` | 动态插桩，无 ABI 保证 |
| 内核函数返回 | `kretprobe:func` | 获取返回值 `retval` |
| 用户函数入口 | `uprobe:lib:func` | 动态插桩用户态 |
| 用户函数返回 | `uretprobe:lib:func` | 获取返回值 |
| 内核 tracepoint | `tracepoint:subsys:event` | 稳定 ABI，有 format 文件 |
| USDT | `usdt:bin:probe` | 用户态静态探针，零未挂载开销 |
| 定时采样 | `profile:hz:99` | CPU 采样（99Hz） |
| 固定间隔 | `interval:s:5` | 每 5 秒触发 |
| 脚本启动 | `BEGIN` | 初始化 |
| 脚本退出 | `END` | 收尾打印 |

## 变量速查

| 类型 | 前缀 | 作用域 | 示例 |
|------|------|--------|------|
| 内置变量 | 无 | 只读上下文 | `pid`, `tid`, `comm`, `nsecs`, `cpu`, `arg0`-`arg5`, `retval` |
| 临时变量 | `$` | 当前 probe 块 | `$start = nsecs;` |
| Map 变量 | `@` | 跨事件持久 | `@[comm] = count();` |

## 聚合函数速查

| 函数 | 用途 | 示例 |
|------|------|------|
| `count()` | 计数 | `@[comm] = count();` |
| `sum(x)` | 求和 | `@[comm] = sum(arg2);` |
| `avg(x)` | 平均值 | `@avg = avg($lat);` |
| `min(x)` / `max(x)` | 极值 | `@max = max($lat);` |
| `hist(x)` | 2 的幂直方图 | `@lat = hist(nsecs - @s[tid]);` |
| `lhist(x,lo,hi,step)` | 线性直方图 | `@lat = lhist($us, 0, 100, 10);` |

## 内置函数速查

| 函数 | 用途 |
|------|------|
| `str(ptr)` | 指针转字符串 |
| `ntop(ip)` | IP 地址转字符串 |
| `kstack` / `ustack` | 内核/用户调用栈 |
| `printf(fmt, ...)` | 格式化打印（低频用） |
| `join(ptr)` | 打印字符串数组 |
| `time(fmt)` | 打印时间戳 |
| `system(cmd)` | 执行系统命令 |
| `exit()` | 退出脚本 |

## 语法结构

```bash
# 基本形式
probe /filter/ { actions; }

# 多探针
probe1 { @a = count(); }
probe2 /pid == 1234/ { @b = sum(arg0); }
END { print(@a); print(@b); }
```

## HFT 常用模式

### 延迟直方图模板

```bash
bpftrace -e '
kprobe:TARGET_FUNC { @start[tid] = nsecs; }
kretprobe:TARGET_FUNC /@start[tid]/ {
    @latency = hist(nsecs - @start[tid]);
    delete(@start[tid]);
}
'
```

### 按进程过滤 + 聚合

```bash
bpftrace -e '
tracepoint:syscalls:sys_enter_sendto
/comm == "myapp"/
{
    @[comm, args->fd] = count();
}
'
```

### 滚动窗口输出

```bash
bpftrace -e '
interval:s:5 {
    print(@count);
    clear(@count);
}
tracepoint:syscalls:sys_enter_read {
    @[comm] = count();
}
'
```

### 常见陷阱

1. **filter 中用 `=` 代替 `==`** — `pid = 1234` 是赋值不是比较，可能不报错但 filter 恒真导致全量输出；始终用 `==` 做比较
2. **临时变量跨 probe 使用** — `$var` 只在当前 probe 块有效，跨事件需用 `@var`；用 `$` 做 Map key 不会持久化
3. **忽视 Map 自动打印导致输出混乱** — 脚本退出时所有 `@` Map 自动打印；如需控制输出，在 `END` 中用 `print(@map)` 或 `clear(@map)`

<details>
<summary>📝 自测题（点击展开）</summary>

1. **kprobe 和 tracepoint 在选型上的优先级是什么？为什么？**

   <details>
   <summary>参考答案</summary>

   优先级：tracepoint > kprobe。Tracepoint 是内核开发者维护的稳定接口，有 format 文件描述字段，跨内核版本兼容；kprobe 依赖内部函数名和参数布局，内核升级后可能改名或改签名导致脚本失效。只有当 tracepoint 不存在时才用 kprobe。
   </details>

2. **`hist()` 和 `lhist()` 有什么区别？何时用哪个？**

   <details>
   <summary>参考答案</summary>

   `hist(x)` 自动按 2 的幂分桶（1, 2, 4, 8, 16...），适合看整体分布形态和量级。`lhist(x, lo, hi, step)` 指定线性区间和步长——如 `lhist($lat, 0, 1000, 10)` 看延迟 0-1000ns 每 10ns 的分布。选择：先 `hist()` 看整体形态定位问题区间，再 `lhist()` 对异常区间做精细分析。HFT 延迟分析两者结合使用。
   </details>

3. **如何实现「每 5 秒输出一次统计结果然后清空」的滚动窗口模式？**

   <details>
   <summary>参考答案</summary>

   用 `interval:s:5` 探针配合 `print()` 和 `clear()`：`interval:s:5 { print(@count); clear(@count); }`。interval 每 5 秒触发一次，打印当前 Map 内容后清空，实现滚动窗口统计。相比脚本退出时一次性输出，滚动窗口能看实时趋势变化。
   </details>

</details>

## 相关章节

- 上一章：[appendix-A-bpftrace单行命令.md](./appendix-A-bpftrace单行命令.md)
- 下一章：[appendix-C-BCC工具开发.md](./appendix-C-BCC工具开发.md)
