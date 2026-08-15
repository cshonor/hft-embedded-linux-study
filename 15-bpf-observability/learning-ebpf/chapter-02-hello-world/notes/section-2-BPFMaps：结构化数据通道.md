# BPF Maps：结构化数据通道

定义在 `uapi/linux/bpf.h`，本质都是 key-value 存储。三大用途：
1. 用户态写配置 → eBPF 读
2. eBPF 存状态 → 另一个（或将来的）eBPF 程序读
3. eBPF 写结果/指标 → 用户态展示

**类型谱系：**
- 数组（key 恒为 4 字节索引）vs 哈希表（任意类型 key）
- 专用优化：FIFO 队列、LIFO 栈、LRU、最长前缀匹配（trie）、Bloom 过滤器
- 对象型：`sockmap`/`devmap`（socket/网卡，供网络程序重定向流量）、`PROG_ARRAY`（存程序 fd，实现尾调用）、map-of-maps
- **per-CPU 变体**：每核一块独立内存——读写免锁，是高性能计数器的标准做法
- 非 per-CPU map 的并发：5.1 起部分 map 支持自旋锁

### 哈希表示例：按 UID 统计 execve 次数

```c
BPF_HASH(counter_table);                    // BCC 宏

int hello(void *ctx) {
  u64 uid;
  u64 counter = 0;
  u64 *p;
  uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;  // 低32位=UID，高32位=GID（掩掉）
  p = counter_table.lookup(&uid);                // 查表，返回值指针；无命中返回 0
  if (p != 0) { counter = *p; }
  counter++;
  counter_table.update(&uid, &counter);
  return 0;
}
```

注意 `counter_table.lookup()` 这种"结构体方法"**不是合法 C**——BCC 先把源码重写成真正的 C 再交给编译器。BCC 的"C"是一门方言。

用户态每 2 秒轮询打印：`b["counter_table"].items()`。`sudo ls` 会计两次：501 执行 sudo 一次、root(0) 执行 ls 一次。

**局限**：用户态必须不停轮询 → 引出事件驱动的缓冲区。
