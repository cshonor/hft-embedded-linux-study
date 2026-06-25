# 第二部分：常见问题与修复 (Common Problems)

### 1. 丢失的事件 (Missing Events)

**挂了 probe 无输出：**

| 原因 | 验证 |
|------|------|
| 事件 **未发生** | `perf stat -e tracepoint:...` 或 `funccount` |
| **静态链接** | 无 PLT — uprobe 挂 libc 失败 |
| **直接 syscall** | 绕过 libc wrapper |
| 过滤器太严 | bpftrace `/filter/` |

```bash
perf stat -e 'syscalls:sys_enter_openat' sleep 5
```

---

### 2. 缺失 / 断裂的堆栈 (Missing Stack Traces)

**现象：** 栈只有 1–2 帧 + **`[unknown]`**。

**根因：** BPF 默认 **帧指针 (frame pointer)** walk — 编译器常 **默认 `-fomit-frame-pointer`**（省略帧指针以微优化），**RBP 不再链栈帧**。

| 错说法 | 正说法 |
|--------|--------|
| 「开了 `-fno-omit-frame-pointer` 导致断栈」 | **omit（省略）帧指针** 导致断栈；修复是 **`-fno-omit-frame-pointer`（不要省略）** |

**修复：**

| 方案 | 说明 |
|------|------|
| **`-fno-omit-frame-pointer`** | 应用/策略二进制 — [Ch 12](../../chapter-12-languages/) |
| **debuginfo + DWARF** | 更准更慢 |
| **ORC / LBR**（内核/perf 能力） | 环境相关、演进中 |
| **libc 断在 `read+0x…`** | 发行版 **libc 无 FP** — [Ch 13 §10](../../chapter-13-applications/) |

**HFT 发布链：** 策略 **.so 必须保留 FP** — 否则 Off-CPU/火焰图 **半盲**。

---

### 3. 缺失符号名 (Missing Symbols)

**现象：** 仅地址 / `[unknown]`，无函数名。

| 环境 | 对策 |
|------|------|
| **JIT (Java/Node)** | **`/tmp/perf-PID.map`** + `jmaps` / perf-map-agent — **实时** 生成 — [Ch 12](../../chapter-12-languages/) |
| **C/C++ ELF** | 勿 **strip**；装 **debuginfo** 包 |
| 内核栈 | `linux-image-*-dbgsym` |

```bash
readelf -s ./my_strategy | head
file ./my_strategy   # not stripped?
```

---

### 4. 追踪时找不到函数 (Missing Functions)

**uprobe 报找不到符号：**

| 原因 | 对策 |
|------|------|
| **内联 (inlining)** | 函数 **不存在独立符号** — 追 **父函数/子函数** |
| 名称 mangling (C++) | `c++filt` |
| 静态函数 | 无导出符号 — 用偏移或换 tracepoint |

**编译验证：** `objdump -d` 看是否只剩父函数体。

---

### 5. 反馈循环 (Feedback Loops) ⚠️

**永远不要追踪「追踪器自身输出路径」。**

| 例子 | 结果 |
|------|------|
| BPF **`printf`** + 同时 trace **`sys_write`** | write → printf → write → **风暴/崩溃** |
| 日志系统 + `write` uprobe | 同理 |

**对策：** 聚合 Map 代替 printf；或 **过滤 trace 自身 PID**；生产用 **结束时的 map 打印**。

---

### 6. 丢弃的事件 (Dropped Events)

**输出/缓冲跟不上事件率：**

| 症状 | 说明 |
|------|------|
| `WARNING: N stack traces could not be displayed` | perf ring buffer 满 |
| map 丢样 | 工具内部限制 |

**修复：**

- **降频** — 换 `funccount`/`hist` 代替 `trace`  
- **增 buffer** — 工具参数 / `sysctl kernel.perf_event_max_stack` 等（视工具）  
- **缩短采集窗口**  
- **只追单 PID**

---
