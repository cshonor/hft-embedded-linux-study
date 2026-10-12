# atomic_ref 与 atomic<shared_ptr>

## atomic_ref：给已有对象"贴上"原子操作

C++17 及以前，`std::atomic<T>` 必须**拥有**它包装的对象——对象是 atomic 的成员。这带来一个结构性限制：内存布局已经定死的场景（共享内存、内存映射文件、网络包缓冲区、定死的 ABI 结构体）没法事后把某个字段变成 atomic。

C++20 `std::atomic_ref<T>` 解决了这件事：**不拥有对象，只提供对既有对象的原子视图**。

```cpp
#include <atomic>

struct alignas(8) ShmHeader {    // 共享内存里的头部，布局不能动
    std::uint64_t write_seq;     // 生产者写的序号
    std::uint64_t read_seq;      // 消费者读的序号
    std::uint64_t padding[6];
};

// C++17：做不到——不能把 shm->write_seq 变成 atomic 又不改布局
// C++20：
void producer_write(ShmHeader* shm) {
    std::atomic_ref seq{shm->write_seq};     // 原子视图贴在已有对象上
    seq.store(42, std::memory_order_release);
}

void consumer_read(ShmHeader* shm) {
    std::atomic_ref seq{shm->write_seq};
    auto v = seq.load(std::memory_order_acquire);
}
```

同一进程内不同线程、甚至**不同进程**映射到同一物理页时，都能各自构造 `atomic_ref` 访问同一对象——这正是共享内存队列（进程间 SPSC ring）的写法。

## atomic_ref 的三条军规

```cpp
// 军规 1：对齐必须满足 atomic_ref 的要求（可能比对 T 本身更严格）
static_assert(alignof(std::atomic_ref<std::uint64_t>) <= alignof(ShmHeader));
// 实践：字段显式对齐到 8 字节，或整个 struct alignas(8)

// 军规 2：对象生命周期必须覆盖 atomic_ref 的使用期
std::atomic_ref<int>* dangling;
{
    int x = 0;
    dangling = new std::atomic_ref<int>(x);
}   // x 死了
// dangling->load();  // UB

// 军规 3：同一对象要么全程 atomic_ref 访问，要么全程普通访问
// 混用 = data race（编译器对普通访问的优化假设不再成立）
int x = 0;
std::atomic_ref<int> ax(x);
ax.store(1);
// int y = x;  // ❌ 普通读和原子写混用，UB
```

## atomic_ref vs atomic：接口差异

| 维度 | `std::atomic<T>` | `std::atomic_ref<T>` |
|---|---|---|
| 拥有对象 | ✅ 成员 | ❌ 引用既有对象 |
| 可拷贝 | ❌ | ❌（但可默认构造空壳再赋） |
| 默认构造值 | 未初始化（C++20 前）/ T{} | 不存在（必须绑定对象） |
| `is_always_lock_free` | 有 | 有（**逐类型**查询） |
| 典型场景 | 普通共享变量 | 共享内存 / 定死布局 / 容器元素 |

容器场景也常用：

```cpp
std::vector<std::uint64_t> counters(64);   // 布局连续，不想包一层 atomic

void bump(size_t lane) {
    std::atomic_ref c{counters[lane]};     // 对容器元素原子++
    c.fetch_add(1, std::memory_order_relaxed);
}
// atomic<uint64_t> 数组会改变 sizeof 和布局；atomic_ref 不动 vector 本身
```

## atomic<shared_ptr>：无锁发布大对象

C++20 前，`shared_ptr` 的引用计数是原子的，但**指向同一对象的两个 shared_ptr 实例本身**不是——多个线程同时读写**同一个** `shared_ptr` 变量是 data race。要么上锁，要么用 `atomic_load(&p)`/`atomic_store(&p, q)` 自由函数（容易写错、被吐槽十年）。

C++20 把这套自由函数收进 `std::atomic<std::shared_ptr<T>>` 特化：

> **⚠ 实现现状实测（2026-10，本机 clang 23.1.0 + libc++）**：`__cpp_lib_atomic_shared_ptr` 宏**未定义**，`std::atomic<std::shared_ptr<T>>` 在 libc++ 下**编译失败**（static_assert 要求 trivially copyable）。libstdc++（gcc 12+）已完整实现。Mac/clang 工具链下用下文的**自由函数版**（C++11 就有，实测可用）；上 Linux/gcc 环境才能用类模板版。

```cpp
#include <memory>
#include <atomic>

// 典型场景：配置/符号表的热路径读 + 低频更新（RCU 模式）
struct SymbolTable { /* 大对象，构造昂贵 */ };

std::atomic<std::shared_ptr<const SymbolTable>> g_table;

// 热路径读者（每 tick 都跑）：
void on_tick() {
    std::shared_ptr<const SymbolTable> tbl = g_table.load(std::memory_order_acquire);
    // tbl 拷贝 → 引用计数 +1，保证使用期间对象不死
    use(*tbl);
}

// 低频写者（symbol 上线时）：
void reload_table() {
    auto fresh = std::make_shared<SymbolTable>(build_from_file());
    g_table.store(std::move(fresh), std::memory_order_release);
    // 旧表在最后一位读者离开后才析构——读多写少的 RCU 语义
}
```

⚠️ 三点诚实标注：

1. **标准不保证 lock-free**——`atomic<shared_ptr>` 允许内部带锁。`g_table.is_lock_free()` 可以查，但主流实现都是带锁的。它的价值是**接口正确性**（不再手写自由函数），不是性能魔法。真无锁 RCU 得自己写双指针切换或 hazard pointer。
2. 每次 `load()` 拷贝 shared_ptr 有**引用计数原子加**的开销（~20ns 级）。tick 级热路径上如果在意，改为"线程启动时 load 一次存线程本地、靠 epoch 定期刷新"。
3. **libc++ 未实现时的替代（Mac/clang 实测可用）**——继续用 C++11 自由函数，语义完全等价：

```cpp
// 自由函数版（libc++ 可用，clang23 实测通过）
std::shared_ptr<const SymbolTable> g_table;   // 普通变量

// 读者：
auto tbl = std::atomic_load(&g_table);
// 写者：
std::atomic_store(&g_table, std::make_shared<const SymbolTable>(build_from_file()));
```

和自由函数版相比，类模板版唯一的改进是"不能忘了用原子访问"（类型强制）——语义、性能完全一样。

## 实战：进程间 SPSC 序号同步

```cpp
// shm_layout.h —— 两个进程共享的内存布局（定死，不能加成员）
struct ShmRing {
    alignas(64) std::uint64_t head;   // 生产者推进
    alignas(64) std::uint64_t tail;   // 消费者推进
    std::byte slots[4096];
};

// producer.cpp
ShmRing* ring = static_cast<ShmRing*>(shm_mmap("/hft_ring"));
std::atomic_ref head{ring->head};

void publish(size_t n_bytes) {
    head.fetch_add(n_bytes, std::memory_order_release);
    // release：先写完 slots 数据，再推进 head，消费者 acquire 读 head 后看到完整数据
}

// consumer.cpp
std::atomic_ref tail{ring->tail};
std::atomic_ref head_ro{ring->head};

size_t available() {
    return head_ro.load(std::memory_order_acquire)
         - tail.load(std::memory_order_relaxed);
}
```

这个模式和 LMAX Disruptor / DPDK ring 的序号机制是同构的——`atomic_ref` 让你在不改共享布局的前提下拿到标准语义（内存序明确、TSan 能查），而不是手写 `volatile` + 内存屏障的黑魔法。

## 自测题

1. `atomic_ref` 解决了 `atomic<T>` 的什么结构性限制？举两个非它不可的场景。
2. 三条军规分别对应什么 UB？为什么"混用普通访问和 atomic_ref"在编译器层面就错了？
3. 为什么 `atomic_ref` 可以用于跨进程同步？依赖操作系统的什么保证？
4. `atomic<shared_ptr>` 相比旧的 `atomic_load(&p)` 自由函数改进了什么？它保证 lock-free 吗？
5. 共享内存 ring 里 head 用 release 推进、消费者 acquire 读取，这对内存序保证的具体内容是什么？（写清楚"先写什么、后看见什么"）
