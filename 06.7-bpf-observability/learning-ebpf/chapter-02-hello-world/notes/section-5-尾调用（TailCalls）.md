# 尾调用（Tail Calls）

> 本节讲什么：尾调用 = eBPF 程序的 `execve`——不回来，直接换下一个程序。本节讲清它的机制、为什么必须存在（512 字节栈）、以及"按 syscall 操作码分发"这个经典用法。

## 1. 定义：类比 execve

进程世界里，`execve()` 把当前进程映像整个换掉，**永不返回**。尾调用对 eBPF 程序干同样的事：

```
普通函数调用:  A ──call──▶ B ──ret──▶ 回到 A 继续跑（B 的栈帧压在 A 上）
尾调用:        A ──tail_call──▶ B 替换 A（A 的栈帧作废）──▶ B 跑完直接退出
```

```c
long bpf_tail_call(void *ctx, struct bpf_map *prog_array_map, u32 index)
```

三个参数：

| 参数 | 含义 |
|---|---|
| `ctx` | 透传上下文——B 拿到 A 收到的同一个 ctx，对 B 来说像自己被事件直接触发 |
| `prog_array_map` | `BPF_MAP_TYPE_PROG_ARRAY` 类型的 map，里面存的是**一组 BPF 程序的 fd** |
| `index` | 选第几个程序 |

**失败语义（设计得妙）**：如果 index 处没有程序，尾调用**失败但程序不死**——调用者从 tail_call 的下一行继续执行。这让"查表分发 + 默认分支"天然融合（下面示例）。

## 2. 为什么必须有它

两个硬限制逼出来的设计：

1. **栈 512 字节**：函数调用链越深越爆栈；尾调用不叠栈——BPF 替换 A，栈从零开始用
2. **单程序指令数上限**（现 100 万）：单程序再大也有限；尾调用链上每个程序都有自己的预算

## 3. 经典用法：按 syscall 操作码分发

目标：只关心几个 syscall，其他 400 个不想逐个 if-else。

**内核侧**：

```c
BPF_PROG_ARRAY(syscall, 300);            // PROG_ARRAY map，300 个槽位

int hello(struct bpf_raw_tracepoint_args *ctx) {
   int opcode = ctx->args[1];             // raw tracepoint 的第2参 = syscall 号
   syscall.call(ctx, opcode);             // BCC 重写为 bpf_tail_call(ctx, syscall, opcode)
   // ↓ 只有尾调用失败（该 opcode 没登记程序）才走到这
   bpf_trace_printk("Another syscall: %d", opcode);
   return 0;
}
```

**用户态配置路由表**（往 map 里塞程序 fd）：

```python
prog_array = b.get_table("syscall")
prog_array[cctypes.SYS_execve] = cctypes.c_int(b.load_func(...exec_fn...))  # 59 = execve
# 噪音 syscall（21 access / 22 madvise / 25 mprotect…）全塞同一个空函数 → 静默吞掉
prog_array[cctypes.SYS_mprotect] = cctypes.c_int(b.load_func(...ignore_fn...))
# 多个 entry 可指向同一程序：222-226（futex 族）全指 hello_timer
```

**执行流程图**：

```
任何 syscall 发生 ─▶ hello ─▶ tail_call(map[opcode])
                                   │
                     ┌─────────────┼─────────────────┐
                     ▼             ▼                 ▼
              map[59]=exec_fn   map[25]=ignore   map[700]=无
              处理 execve      空函数直接返回    失败→ 回到 hello 打印默认消息
```

这就是**内核态消息路由**：事件 → 查表 → 分发到处理器，表还是运行时可改的（用户态随时换 map 里的程序）。XDP 的多级包处理（解析 → 过滤 → 转发，每级一个程序）就是这套结构的网络版（第 8 章）。

## 4. 限制

| 限制 | 说明 |
|---|---|
| 链深上限 **33** | 内核防止死循环（33 层内必须终结） |
| 程序类型必须一致 | 尾调用的程序和调用者同类型（kprobe 程序只能尾调 kprobe 程序） |
| JIT 依赖 | 子程序内尾调用需 JIT 支持：写书时仅 x86，**ARM 6.0 才加**——树莓派（ARM64, 6.18）无碍 |
| 与 BPF-to-BPF 互斥 | 5.10 前互斥；**5.10 起解除** |

## 5. 三种代码复用机制对比收尾

| | 内联 | BPF-to-BPF 调用 | 尾调用 |
|---|---|---|---|
| 机制 | 编译期复制 | 运行期 call/ret | 运行期整程序替换 |
| 栈 | 不占 | 共享栈（深了爆） | 从零开始 |
| 数量限制 | 无 | 栈容量 | 33 层 |
| 返回 | — | 会返回 | 永不返回 |
| 工具链 | BCC ✓ libbpf ✓ | libbpf 才支持 | BCC ✓ libbpf ✓ |

---

**衔接**：第 2 章完——hello world、map、ring buffer、代码组织全通了。第 3 章撕开 BCC 的黑盒：手工编译、加载、挂载，看清每一步。
