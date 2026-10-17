# span 子视图与网络包零拷贝解析

## 子视图三件套：subspan / first / last

span 最大的价值不是"看一眼整个缓冲区"，而是**不开新内存地切出一段**。网络协议解析就是一层层切：

```cpp
#include <span>
#include <cstddef>
#include <cstdint>

std::byte packet[1500];   // 网卡收上来的原始帧
std::span<std::byte> buf(packet);

// first(n)：前 n 字节（以太网头 14 字节）
auto eth = buf.first(14);

// subspan(off, len)：从 off 开始切 len 字节（IP 头通常 20 字节）
auto ip = buf.subspan(14, 20);

// subspan(off)：从 off 到结尾（TCP 段 = 剩下的全部）
auto rest = buf.subspan(14 + 20);

// last(n)：尾部 n 字节（FCS 校验 4 字节）
auto fcs = buf.last(4);
```

三次切分，**零拷贝、零分配**——每个子 span 只是 `(指针, 长度)` 一对值，8~16 字节，直接放寄存器。

## 静态 extent：把长度编进类型

```cpp
// 动态 extent（默认）：长度是运行期成员，sizeof(span) == 16
std::span<std::byte> dyn(packet);

// 静态 extent：长度是类型的一部分，sizeof == 8（只有指针）
// ⚠ 必须用编译期版本 first<14>()——尖括号！
std::span<std::byte, 14> eth_fixed{buf.first<14>()};

// first(14)（圆括号、运行期参数）返回的是动态 extent span，
// 转不进 span<std::byte, 14>：
//   parse_eth(buf.first(14));   // ❌ 编译错误（clang23+libc++ 实测）
//   parse_eth(buf.first<14>()); // ✅ extent 匹配
static_assert(decltype(buf.first<14>())::extent == 14);
static_assert(decltype(buf.first(14))::extent == std::dynamic_extent);

eth_fixed.size();        // 编译期常量 14，不占内存
// eth_fixed = buf.first(20);  // 编译错误：长度不匹配
```

协议头长度固定时（以太网 14、IPv4 最小 20、TCP 最小 20），用静态 extent：

- `sizeof` 减半——span 能整个放进一个寄存器传参
- 长度不匹配直接**编译期**抓住，不用运行期检查

```cpp
// 函数签名即协议：只接受恰好 14 字节的以太网头
void parse_eth(std::span<const std::byte, 14> header);

parse_eth(buf.first<14>());   // ✅ 尖括号 → 静态 extent
// parse_eth(buf.first(14));  // ❌ 圆括号 → 动态 extent，类型不匹配
```

> **实测（clang 23 + libc++，-std=c++20）**：圆括号 `first(14)` 返回 `span<T, dynamic_extent>`，无法隐式转 `span<const T, 14>`；尖括号 `first<14>()` 的 extent 是编译期常量 14，`sizeof` 退化为 8 字节（仅指针）。

## as_bytes / as_writable_bytes：任意类型 ↔ 字节视图

```cpp
struct alignas(8) Tick {
    int64_t ts_ns;
    int32_t symbol_id;
    double  price;
    int64_t qty;
};

Tick t{1'752'000'000'000'000'000LL, 700, 182.34, 100};

// 对象 → 字节：发上网线前看一眼它的内存布局
auto bytes = std::as_bytes(std::span{&t, 1});
send(fd, bytes.data(), bytes.size(), 0);

// 字节 → 对象：收包后直接"贴"到结构体上解析（零拷贝反序列化）
std::byte rx[sizeof(Tick)];
auto writable = std::as_writable_bytes(std::span{rx});
recv(fd, writable.data(), writable.size(), 0);

const Tick& parsed = *reinterpret_cast<const Tick*>(rx);
```

⚠️ 三个前提，缺一就是 UB：

1. 类型是 **trivially copyable**（没有虚函数、自定义构造）
2. **对齐**满足（`alignof(Tick)` vs 缓冲区地址）
3. 跨机器要考虑**字节序和 padding**——同构集群内部总线这么玩没问题，跨平台老老实实序列化

## 实战：行情包的零拷贝解析骨架

```cpp
// 协议：[msg_type:1][symbol_id:4][price:8][qty:8]
struct FeedHandler {
    void on_packet(std::span<const std::byte> pkt) {
        if (pkt.size() < 21) return;   // 唯一一次长度检查

        uint8_t type = static_cast<uint8_t>(pkt[0]);

        auto body = pkt.subspan<1>();  // 编译期偏移 → 静态 extent 子 span

        int32_t sym;
        double  price;
        int64_t qty;
        std::memcpy(&sym,   body.data(),      4);
        std::memcpy(&price, body.data() + 4,  8);
        std::memcpy(&qty,   body.data() + 12, 8);
        // memcpy 编译期定长 → 编译器优化成单条 load，比 reinterpret_cast 安全

        dispatch(type, sym, price, qty);
    }
};
```

要点：边界检查只做一次，之后每个字段的偏移都是编译期常量。对比 `char*` + 手动偏移的写法，span 版本多了长度信息、少了所有悬垂算术。

## span 的坑（HFT 场景高发）

```cpp
// 坑 1：悬垂——span 不拥有内存，指向的 vector 扩容后全废
std::span<int> s;
{
    std::vector<int> v{1, 2, 3};
    s = v;
    v.push_back(4);    // 可能 reallocate
}
// s[0];               // UB：s 还指着旧内存

// 坑 2：临时对象
// auto bad = std::span{std::vector<int>{1,2,3}};  // C++20 禁止：右值 range 不能构造 span
                                                  // （除非元素是 const 的意外情况，别依赖）

// 坑 3：const 粒度
std::span<const int> read_only;   // 元素不可改 ✅
const std::span<int>  fixed_view; // span 本身不可改（不能 reseat），元素仍可改
```

**纪律：span 只在"下层缓冲区生命周期严格更长"的作用域里用**——收包循环内、函数参数，别存进成员变量跨帧使用。

## 自测题

1. `subspan<1>()`（编译期偏移）和 `subspan(1)`（运行期偏移）返回类型有什么区别？对 `sizeof` 有什么影响？
2. 静态 extent 的 span 相比动态 extent 省什么？代价是什么？
3. 用 `as_bytes` + `reinterpret_cast` 做零拷贝反序列化的三个 UB 前提是什么？
4. 为什么实战代码里用 `memcpy` 取字段而不是直接 `reinterpret_cast<const double*>(p)` 解引用？（提示：对齐 + strict aliasing）
5. span 悬垂的两种典型场景？为什么行情处理里"span 不存成员变量"是安全的纪律？
