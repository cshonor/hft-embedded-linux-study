# BPF Maps：结构化数据通道

> 本节讲什么：map 是 eBPF 程序与外界交换结构化数据的唯一正规通道。本节过一遍类型谱系（知道选型表在哪个头文件），再逐行精读"按 UID 统计 execve"这个经典例子。

## 1. 为什么需要 map

eBPF 程序跑在内核里，但：用户态要给它传配置、它要存跨事件的状态、结果要给用户态看。普通进程有全局变量/堆/文件——eBPF 程序都没有（严格说，全局变量也是用 map 实现的，第 3 章 §5）。**map = eBPF 的"内存 + IPC"合体**。

定义全在 `uapi/linux/bpf.h`（`BPF_MAP_TYPE_*` 枚举，30 来种），本质都是 key-value 存储。三大用途：

```
用户态 ──写配置──▶ ┌──────────────┐ ──读配置──▶ eBPF 程序 A
                  │     map      │ ──存状态──▶ eBPF 程序 B（跨事件/跨程序）
用户态 ◀──读指标── │ (内核键值库) │ ◀──写结果── eBPF 程序 A
                  └──────────────┘
```

1. 用户态写配置 → eBPF 读（下发过滤规则）
2. eBPF 存状态 → 另一个（或将来的）eBPF 程序读（跨事件记状态）
3. eBPF 写结果/指标 → 用户态展示（计数器、直方图）

## 2. 类型谱系（选型地图）

| 家族 | 成员 | 适用 |
|---|---|---|
| 基础 | **数组**（key 恒为 4 字节索引）/ **哈希表**（任意 key） | 90% 场景：计数用数组（UID≤某值时）、任意 key 用 hash |
| 专用优化 | FIFO 队列 / LIFO 栈 / LRU / 最长前缀匹配 trie / Bloom 过滤器 | 特定算法需求 |
| 对象型 | `sockmap`/`devmap`（socket/网卡，供网络程序重定向流量）、`PROG_ARRAY`（存程序 fd → 尾调用 §5）、map-of-maps | 网络数据面 |
| **per-CPU 变体** | 数组/哈希都有 per-CPU 版 | 高性能计数器标准做法（见下） |

**per-CPU 是什么**：每个 CPU 核各有一块独立 map 存储，各核只读写自己的副本 → **天然免锁**。用户态读时 BCC 自动汇总各核值。代价：内存 × 核数、跨核读不即时一致。原理与用户态无锁 per-core 计数器一致（HFT 关联 §7 展开）。

非 per-CPU map 的多核并发：5.1 起部分 map 支持自旋锁（`bpf_spin_lock`，须内嵌在 value 结构里且需 BTF——你的 rpi 内核没有，第 5 章坑点）。

## 3. 哈希表示例精读：按 UID 统计 execve

```c
BPF_HASH(counter_table);                    // BCC 宏：默认 u64 key → u64 value

int hello(void *ctx) {
  u64 uid;
  u64 counter = 0;
  u64 *p;
  uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;  // ① 低32位=UID，高32位=GID（掩掉）
  p = counter_table.lookup(&uid);                // ② 查表，返回 value 的指针；无命中返回 NULL
  if (p != 0) { counter = *p; }                  // ③ 必须判空！
  counter++;
  counter_table.update(&uid, &counter);          // ④ 写回
  return 0;
}
```

四个新手要点：

- **① 掩码方向**：`bpf_get_current_uid_gid()` 返回 u64，**UID 在低 32 位**——和 `pid_tgid`（PID 在**高** 32 位）方向相反，两个 helper 最容易搞混，写错了统计维度就全错
- **② 返回的是指针**：map 的 value 存在内核内存里，lookup 给你一个指向它的指针，读它就是直接读内核内存
- **③ 判空不是可选项**：`lookup` 查不到返回 NULL，不判空直接解引用 = verifier 直接拒绝（第 6 章 §3.5 的经典报错 `map_value_or_null`）。这段代码没加锁，多核并发下 read-modify-write 有竞态（丢失增量）——计数精确场景换 per-CPU 版
- **④ key/value 都传地址**：helper 参数约束（`ARG_PTR_TO_MAP_KEY`）

**BCC 方言警告**：`counter_table.lookup(&uid)` 这种"结构体方法"**不是合法 C**——BCC 先做文本重写成真正的 `bpf_map_lookup_elem()` 调用再交给编译器。BCC 的"C"是一门方言，换 libbpf 时这些全要手写重改（第 5 章迁移实战）。

## 4. 用户态读法

```python
while True:
    sleep(2)
    for k, v in b["counter_table"].items():
        print(f"UID {k.value}: {v.value}")
    b["counter_table"].clear()      # 清零重新统计
```

小实验：跑起来后执行 `sudo ls`，会看到两个条目各 +1：普通用户(如 501) 执行 sudo 一次、root(0) 执行 ls 一次——sudo 的 setuid 机制在 eBPF 视角下的直接呈现。

## 5. 局限 → 下一节

map 是"拉"模式：用户态必须不停轮询。事件稀疏时轮询浪费、事件密集时又不够实时。需要"推"模式：内核有事件时主动通知用户态——环形缓冲区登场。

---

**衔接**：下一节 Perf/Ring Buffer——eBPF 世界的事件总线。
