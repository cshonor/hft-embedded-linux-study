# 4. 四大多用途工具

### 选型速查

| 工具 | 回答什么 | 输出形态 | 适合事件频率 |
|------|----------|----------|--------------|
| **`funccount`** | 谁被调了多少次？ | 计数表 | **高** |
| **`stackcount`** | 哪些栈路径触发了事件？ | 栈 + 计数 → **火焰图** | **中高** |
| **`trace`** | 每次事件的细节（参数/返回值）？ | **逐行打印** | **低** |
| **`argdist`** | 参数/返回值分布？ | 频率或 **2 的幂直方图** | **中高** |

```
高频事件 ──────────────────────────────────────────► 低频事件
  funccount / argdist (内核 Map 聚合)     trace (逐行)
              stackcount (栈聚合)
```

### `funccount` — 事件频率统计

在内核 BPF 程序里对事件 **`++`**，结果存 Map；用户态只读汇总表。

```bash
# 统计内核函数调用次数（示例）
sudo funccount-bpfcc 'vfs_read'

# 统计 tracepoint
sudo funccount-bpfcc -t 'syscalls:sys_enter_read'

# 统计 USDT（若进程带探针）
sudo funccount-bpfcc -p $(pidof myapp) 'u:myapp:probe_name'
```

**要点：** 不把每条事件送到用户态 — **海量调用** 时仍可用。  
**HFT：** 验证「这条 syscall 是否在策略循环里被疯狂调用」；比 `strace` 轻，但仍非零开销，勿长期挂在最低延迟核。

### `stackcount` — 栈频率 + 火焰图

统计 **导致某事件的完整内核/用户栈** 及次数；输出可喂给 **火焰图 (Flame Graphs)**。

```bash
sudo stackcount-bpfcc -f 'vfs_write' > out.stacks
# 用 FlameGraph 工具折叠（书内 / Brendan Gregg 仓库）
```

**与 `profile` 区别（直觉）：**

| | `stackcount` | `profile` |
|---|--------------|-----------|
| **触发** | 你指定的 **事件**（如某函数入口） | **定时采样** CPU |
| **问题** | 「谁走了这条路径？」 | 「CPU 时间在哪儿？」 |

→ 火焰图原理：[Ch 2 § 火焰图](../../chapter-02-technology-background/)

### `trace` — 逐事件详情

打印 **每次** 命中的自定义信息：函数参数、返回值、时间戳等。

```bash
sudo trace-bpfcc -p $(pidof myapp) 'u:myapp:entry %d %s', arg1, arg2
```

| 适合 | 不适合 |
|------|--------|
| 低频路径、启动阶段、偶发错误 | 高频 `read`/`send`、每 tick 都触发的 probe |

**HFT：** 仅用于 **复现窗口内的短跑**（如单次下单路径验证）；高频路径用 `funccount` / `argdist` 或 bpftrace 聚合。

### `argdist` — 参数/返回值分布

在内核用 Map 做 **频率计数** 或 **2 的幂次方直方图** — 看「参数通常多大」。

```bash
# 分布：read() 的 count 参数
sudo argdist-bpfcc -C 'p::sys_read(size_t):size_t size'

# 直方图模式（示意，具体语法见 man）
sudo argdist-bpfcc -H 'p::malloc:u64'
```

**HFT：** 看 `recv` 长度分布、`write` 大小 — 判断是否在发大量小包（与 [Ch 10 网络](../../chapter-10-networking/) 衔接）。

---
