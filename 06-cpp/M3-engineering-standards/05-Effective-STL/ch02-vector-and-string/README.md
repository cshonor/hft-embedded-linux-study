# 第 2 章 vector 和 string

**vector and string** — Items 12–17

## 本章讲什么

`vector` 是 STL 最常用的容器，`string` 是最常用的字符串。本章聚焦二者的内存管理（`capacity`/`reserve`/`shrink_to_fit`）、与 C API 的互操作、以及 `vector` 与 `string` 数据的可互换性——这些细节直接决定热路径的分配次数与延迟稳定性。

---

## 各 Item 要点

### Item 12–13：理解 `vector`/`string` 的容量与扩容

- `size()` = 当前元素数；`capacity()` = 已分配可容纳数；`reserve(n)` 预留容量（不改 size）；`shrink_to_fit()` 请求收缩（非强制）。
- `vector` 扩容策略通常是**翻倍**——均摊 O(1) 插入，但单次扩容有 O(n) 拷贝/移动尖峰。
- 扩容时迭代器、指针、引用**全部失效**（内存搬迁）。这是 `vector` 最常见的 UB 来源。

**HFT 铁律**：知道峰值容量就 `reserve`，让热路径零扩容、零迭代器失效。

### Item 14：用 `reserve` 避免不必要的重新分配

```cpp
std::vector<Tick> ticks;
ticks.reserve(estimate);          // 一次性分配
for (...) ticks.push_back(tick);  // 零扩容
```

`reserve` 是 `vector` 性能调优的第一手段。对 `unordered_map` 同理——`reserve(bucket_count)` 避免 rehash 尖峰。

### Item 15：注意 `string` 实现的多样性

不同标准库实现的 `string` 布局不同：libstdc++ 用 COW（C++11 前）/ SSO（小字符串优化）；libc++ 用 SSO；MSVC 用 SSO。`sizeof(string)` 从 8 到 32 字节不等。跨平台二进制共享时不能假设 `string` 布局——用 `const char*` 或定长缓冲。

**SSO（Small String Optimization）**：短字符串直接存在对象内部（无堆分配），长字符串才堆分配。阈值通常 15~22 字节。HFT 短 symbol（如 "BTCUSDT"）享受 SSO 零分配。

### Item 16：将 `string`/`vector` 数据传给旧 C API

```cpp
std::string s = "hello";
legacy_c_func(s.c_str());              // 只读 C 字符串

std::vector<char> buf(256);
legacy_read(buf.data(), buf.size());   // 可写缓冲
```

`c_str()` / `data()` 取连续内存传给 C API。C++11 起 `string` 与 `vector` 都保证连续存储，`data()` 等价于 `&v[0]`。

### Item 17：交错使用 `vector` 和 `string` 数据

`vector<char>` 和 `string` 都是一段连续字符，可互相借用：`string(v.begin(), v.end())` 构造、`std::string_view`（C++17）零拷贝指向二者。但 `string` 有 `\0` 终止语义，`vector<char>` 没有——二进制数据用 `vector<char>` 或 `string_view`，别用 `string`（见《C 和指针》ch9 的 strlen 陷阱）。

---

## HFT 关联

- **`reserve` 消除扩容尖峰**：行情 tick 缓冲按峰值 `reserve`，热路径零扩容。这是 HFT `vector` 调优第一课。
- **SSO 与 symbol**：交易对 symbol 短（"AAPL"、"ESU5"），SSO 让 `string` 零堆分配。但长 symbol（"BTC-PERP-20240927"）超 SSO 阈值会堆分配——可考虑 `string_view` + 外部缓冲。
- **`vector<char>` vs `string` 解析**：FIX/二进制协议用 `vector<char>` 或 `string_view`，避免 `string` 的 `\0` 截断语义。
- **C API 互操作**：DPDK / syscall 接口要 `data()`/`c_str()` 取裸指针，注意返回前容器不能扩容（否则指针失效）。

---

## 自测题

1. `vector` 扩容时哪些会失效？为什么 `reserve` 能消除热路径的扩容尖峰？
2. SSO 是什么？短字符串为什么能零堆分配？阈值大概是多少？
3. 把 `string` 数据传给 C API 用什么方法？传可写缓冲用 `vector` 的什么接口？
4. 为什么二进制数据不该用 `string` 存？应该用什么？
5. `sizeof(std::string)` 在不同实现下为什么不同？跨平台共享要注意什么？
