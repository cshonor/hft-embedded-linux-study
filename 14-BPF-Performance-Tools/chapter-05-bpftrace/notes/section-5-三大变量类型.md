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

---
