# 09 · 序列化陷阱（Protobuf 等）

<a id="pnp-09-goal"></a>

## 目标

跨语言、跨版本、内存对齐；**UNP 少讲**，PNP 工程向重点。

<a id="pnp-09-unp"></a>

## UNP 对照

- 无直接章节；网络字节序见 Ch3、`htonl`

<a id="pnp-09-concepts"></a>

## 概念详解

### 1. 为什么结构体不能直接 `memcpy` 上线

把内存里的 `struct` 原样发送，四个独立炸点：

```cpp
struct Order {
    int32_t  id;        // 8B 处还轮不到它——看 padding
    int64_t  price;     // 8 字节成员要求 8 字节对齐 → id 后面垫 4 字节
    char     side;      // 1 字节
    // 编译器在末尾再垫 7 字节让 sizeof 是 8 的倍数
};                      // sizeof = 24，线上真正有用的只有 13 字节
```

| 炸点 | 表现 |
|------|------|
| **padding** | 不同编译器/开关下填充不同，两端布局不一致 → 字段错位 |
| **字节序** | x86 小端 vs 网络大端，整数字段直接镜像互换 |
| **类型宽度** | `long` 在 Linux LP64 是 8 字节、Windows LLP64 是 4 字节；`time_t`、指针字段更不能上线 |
| **对齐访问** | 收到字节流 `(int64_t*)(buf+1)` 这类未对齐 cast 在 x86 能跑，ARM/Sparc 直接 SIGBUS |

### 2. 正确的二进制纪律（wire format 规则）

1. 只用 **定宽类型**：`int8/16/32/64_t`，禁止 `long`/`int`/指针上协议
2. **字节序显式化**：全协议统一（大端=网络序是传统；FIX/SBE 用小端），收发两端各转一次
3. **无隐式 padding**：要么 `#pragma pack(1)`（代价：未对齐访问），要么手写字段级 encode/decode（推荐，编译器优化后基本免费）
4. **版本协商**：帧头带版本号 + 长度；新版本只能加字段不能改老字段含义

```cpp
// 手写 encode/decode：显式、可移植、零歧义
void encodeOrder(char* p, const Order& o) {
    p = put_be32(p, o.id);         // 4B 大端
    p = put_be64(p, o.price);
    *p++ = o.side;
}
Order decodeOrder(const char* p) {
    Order o;
    o.id    = get_be32(p);  p += 4;
    o.price = get_be64(p);  p += 8;
    o.side  = *p;
    return o;
}
constexpr size_t kOrderWireSize = 4 + 8 + 1;            // id + price + side
static_assert(kOrderWireSize == 13, "wire size frozen"); // 冻结线上尺寸
```

### 3. protobuf：怎么编码的，怎么演化的

编码要点（varint + tag）：

- 每个字段 = `tag(varint) = field_number << 3 | wire_type` + 值
- 整数用 **varint**（每 7 位一组，高位表继续）：小数字 1 字节，代价是大数最多 10 字节
- 未设字段 **完全不占线上字节**；接收端遇到未知 field number 存进 unknown fields 原样保留

演化规则（兼容性的生命线）：

| 改动 | 安全？ |
|------|--------|
| 新增字段 | 安全（老端跳过，新端用默认值） |
| 删除字段 | **field number 永不复用**（`reserved`）即安全 |
| 改字段号 | 等于删+加，数据"消失" |
| 改类型 int32↔int64 | varint 域内兼容 |
| 改 required（proto2） | 灾难：required 永远别用 |

### 4. HFT 的选择：SBE / ITCH 为什么不是 protobuf

| | protobuf | SBE（FIX Simple Binary Encoding） | ITCH（交易所原生） |
|---|----------|-----------------------------------|--------------------|
| 布局 | varint 流，**必须顺序解析** | **固定偏移**，字段直接按 offset 访问 | 定长消息，按消息类型查表 |
| 解码开销 | varint 循环 + 分支 | 近零：读到 offset 直接用（flyweight，不拷贝） | 近零 |
| 带宽 | 小数字紧凑 | 略大（定长字段） | 定长 |
| 适用 | 配置/RPC/管理面 | **热路径行情/订单** | **交易所原生行情** |

一句话：**管理面用 protobuf 的灵活性，热路径用定长二进制的确定性**。

### 5. 定长帧 + 零拷贝解码（把 02/08/09 串起来）

```
[2B 类型][8B 序列号][定长 payload]  ← 组播 UDP 数据报天然保边界（08）
                                          ↓
                        校验长度 → 按类型偏移表取字段（零拷贝，SBE 式）
```

<a id="pnp-09-code"></a>

## C++ 示例：packed 结构 + 静态断言（知道何时可用）

```cpp
// wire.h — 定长线上格式：pack(1) + static_assert 双保险
#pragma pack(push, 1)                 // 线上布局：13 字节，无 padding
struct WireOrder {
    uint32_t id;                      // 大端（encode 时转）
    uint64_t price;                   // 大端
    char     side;                    // 'B'/'S'
};
#pragma pack(pop)

static_assert(sizeof(WireOrder) == 13, "wire format frozen");
static_assert(sizeof(uint32_t) == 4 && sizeof(uint64_t) == 8,
              "wire types must be fixed-width");

// 注意：pack(1) 后对 WireOrder* 取成员地址做 8 字节加载是未对齐访问——
// ARM 上要么 memcpy 取值，要么编译器 -fpack-struct 后由编译器生成安全访问
inline uint64_t load_be64(const void* p) {
    uint64_t v;
    memcpy(&v, p, 8);                 // memcpy 定宽：编译器优化成单条加载
    return __builtin_bswap64(v);      // 小端主机转大端（GCC/Clang）
}
```

<a id="pnp-09-kernel"></a>

## 底层视角

- **字节序根源在 CPU**：x86 `mov` 直接按小端解释寄存器；网络协议族历史选择大端（先传高位字节）。`htonl` 在 x86 上编译成 `bswap` 指令（~1 cycle）——字节序转换在 encode/decode 里根本不是性能问题，varint 的分支循环才是
- **对齐根源在访存通路**：CPU 按对齐边界访问 cache line；未对齐访问 x86 靠硬件拆成两次（慢），部分架构直接异常。MMIO/网卡 DMA 描述符同样要求对齐——嵌入式背景（振鹏的老朋友）在 DMA ring buffer 上见过同一件事
- **零拷贝链条**：`recvfrom MSG_DONTWAIT` 后不 memcpy 进业务对象，直接在收包缓冲上按偏移解出字段——配合 [13 DPDK](../../13-dpdk/) 用户态收包，"网卡到策略"全程零拷贝是 HFT 解码的终极形态

<a id="pnp-09-pitfalls"></a>

## 坑点

- 结构体 `memcpy` 上线（对齐、32/64 位）
- protobuf `optional` / `packed` 与旧客户端兼容
- field number 复用（proto2 的 required 心魔）
- `long`/`time_t`/指针字段进协议——Windows 客户端连上来的那天爆炸
- varint 的 64 位负数占 10 字节（sint64/zigzag 才是正解）
- 忘 `static_assert` 冻结线上尺寸：一次"顺手加个字段"毁掉所有兼容承诺
- 消息加长后老版本长度校验拒绝——长度字段语义（总长 vs payload 长）要写进协议文档

<a id="pnp-09-hft"></a>

## HFT 关联

| 场景 | 关系 |
|------|------|
| 行情解码 | ITCH/MDP 3.0（CME）都是定长二进制，接收线程每秒百万级消息解码——varint 类格式直接出局 |
| 策略延迟 | 解码在关键路径上：offset 直取 + `bswap` ≈ 纳秒级；protobuf 顺序解析 ≈ 百纳秒~微秒级 |
| 帧协议 | 本模块 [02 长度前缀](./02_TCPByteStream.md) + [08 组播保边界](./08_UDP_Multicast.md) + 本节定长格式 = 完整的 HFT 行情接收协议栈 |
| 测试纪律 | 编解码 golden 文件 + 字节级 diff，防"顺手优化"破坏线上兼容 |

<a id="pnp-09-quiz"></a>

## 自测题

1. 上面 `Order` 结构体 x86-64 默认对齐下 `sizeof` 是多少？画出 padding 分布。
2. 为什么 `long` 不能进 wire format？给出它在两种主流平台上的宽度。
3. protobuf 新增字段，旧客户端行为是什么？未知字段存哪了、何时有用？
4. varint 编码 `-1`（int64）占几字节？`sint64` 呢？
5. SBE 的"固定偏移"为什么能零拷贝解码？代价是什么？

<a id="pnp-09-refs"></a>

## 交叉引用

- 上一篇：[08 UDP/组播](./08_UDP_Multicast.md)
- [02 粘包（帧协议）](./02_TCPByteStream.md) · [13 DPDK](../../13-dpdk/) · [14 HFT 工程](../../14-hft-engineering/) · [18 Rust 量化（序列化对比）](../../18-rust-quant/)
