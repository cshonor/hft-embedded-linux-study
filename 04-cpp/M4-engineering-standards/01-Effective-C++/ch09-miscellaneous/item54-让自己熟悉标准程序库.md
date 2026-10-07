# 条款 54：让自己熟悉标准程序库

## 本节讲什么

**Familiarize yourself with the standard library.** 原书（2005）这条讲的是
TR1——那是 C++11 的前夜。今天它的真正含义是：**标准库已经膨胀到
"不知道它有什么"成为最常见的重复造轮子原因**。本条按"必须条件反射"的
优先级梳理现代标准库版图（C++11→23），并标注哪些轮子坚决不该自己造。
（本条已按 2026 视角重写——TR1 内容全部进了 std::，不再单独存在。）

← 上一条 [item53 别忽略警告](./item53-不要轻易忽略编译器的警告.md)；
下一条 [item55 熟悉 Boost](./item55-让自己熟悉Boost.md)。

---

## 1. TR1 的现代答案（历史一句话）

原书列举的 TR1 组件——`shared_ptr`/`function`/`bind`/`tuple`/`regex`/
`unordered_map`/`array`/`type_traits`/`random`——**全部在 C++11 进了标准库**，
就是今天的 `<memory>`/`<functional>`/`<tuple>`/`<regex>`/
`<unordered_map>`/`<array>`/`<type_traits>`/`<random>`。
TR1 作为名词已死，本条的精神永存：**每三年标准库大一次，
"熟悉标准库"是持续义务**。

## 2. 现代标准库的"条件反射"清单（按层）

**容器与视图**（写容器前先问标准库有没有）：

| 需求 | 标准答案 | 版本 |
|---|---|---|
| 只读视图（零拷贝子串/子数组） | `string_view` / `span` | C++17/20 |
| 异构值（封闭类型集） | `variant` + `visit` | C++17 |
| 可空值 | `optional` | C++17 |
| 类型擦除单值 | `any`（冷路径） | C++17 |
| 编译期元组 | `tuple`/`pair` + 结构化绑定 | C++11/17 |

**算法**（手写循环前先翻 `<algorithm>`/` <numeric>`）：
`sort/find/count` 只是起点——`lower_bound`（有序二分）、`nth_element`（第 k 大）、
`partial_sort`（TopN）、`accumulate`/`reduce`（求和）、`transform_reduce`（点积）、
`iota`（序列生成）、`scan` 系（前缀和 C++17）——
**手写的循环 80% 有现成算法**，且算法版自带优化（并行执行策略 C++17）与正确性验证。

**工具**：
- `<chrono>`：时间的一切（热路径计时 `steady_clock`，→ 06.6.5 ch02）
- `<random>`：分布引擎（别再 `rand() % n`——有偏，→ 01-c 的相关讨论）
- `<bit>`（C++20）：`popcount`/`countl_zero`/`bit_cast`/`endian`——
  位操作与 type punning 的合法化（→ 19.5 ① 提到的 bit_cast）
- `<expected>`（C++23）：错误码与异常的现代中间态（热路径错误的候选答案，→ 18.1 HFT）

## 3. 不该自己造的轮子（高频重灾区）

| 自造轮子 | 标准答案 | 自造的代价 |
|---|---|---|
| 字符串分割/拼接/前后缀判断 | `string_view` + `starts_with/ends_with`（C++20） | 手写循环的边界 bug 日常 |
| 线程池/锁封装 | `<thread>` + `<mutex>` + `<future>`（骨架） | 手写锁的 ABA/内存序坑（→ M3 并发） |
| `max(a,b,c...)` 手写 | `std::max`（initializer_list 版） | 重复评估参数的陷阱 |
| 手写 hash 组合 | `hash` + 位混合（或直接用 `boost::hash_combine` 模式） | 散列质量差导致 unordered_map 退化 |

## HFT 关联

- **热路径标准库的白名单**（零分配/无锁/可预测）：`array`、`string_view`、
  `span`、`optional`、`variant`、`chrono::steady_clock`、`<bit>`——
  这些随便用；`unordered_map`（分配+哈希抖动）、`regex`（回溯爆炸）、
  iostream（锁+locale）**不进热路径**（→ 06.6.5/06.6.5 ch05 的实测口径）
- `std::expected`（C++23）是热路径错误处理的新标准答案候选：
  比异常便宜、比错误码安全（强制检查）——新代码的错误通道优先它（→ 18.1）
- `<bit>` 的 `countl_zero`/`popcount` 编译到单条指令（LZCNT/POPCNT）——
  订单簿位图查找的标准零件，手写循环找最高位是时代眼泪

## 代码自测

**题目 1：** 原书的 TR1 条款在今天应该怎么读？

<details>
<summary>参考答案</summary>

TR1 的组件已全部进入标准库（C++11 起），"熟悉 TR1"自动转化为
"熟悉标准库"——而且义务更重：标准库每三年扩容（11→14→17→20→23），
"不知道标准库有"是最常见的重复造轮子原因。
阅读建议：本条的精神不变（投资标准库的 ROI 永远最高），
清单要按现代版图更新（string_view/span/variant/optional/`<bit>`/expected
这些 TR1 时代不存在的东西，今天都是一线工具）。

</details>

**题目 2：** 热路径上想用哈希表存"合约代码→订单簿"，`std::unordered_map` 有什么问题？
标准库内的替代思路？

<details>
<summary>参考答案</summary>

unordered_map 的热路径三宗罪：① 节点分配（每个元素一次 new——抖动，
→ 19.1 HFT）；② 哈希计算不确定（字符串哈希 + 冲突链遍历，延迟不可预测）；
③ 缓存不友好（节点分散在堆上，指针跳转即 cache miss）。
标准库内的替代：**`vector` + 二分查找**（合约数有限且启动时已知——
排序后 `lower_bound`，缓存友好、零分配、可预测）；
或**开放寻址自定义表**（标准库外，如 `ankerl::unordered_dense`/abseil flat_map——
连续存储的哈希表，这是"标准库不够时"该去的方向，不是自己手撸）。
选型口诀：热路径容器第一问永远是"分配行为和缓存行为可预测吗"。

</details>

**题目 3：** `std::string_view` 为什么能成为热路径白名单，而 `std::string` 不能？

<details>
<summary>参考答案</summary>

`string_view` = **指针 + 长度**（16 字节 POD）：不拥有内存、零分配、
按值传递零成本——它是"字符串的只读句柄"，构造/拷贝/析构全是免费的。
`std::string` 拥有内存：短字符串 SSO 免费，但超过 SSO（15/22 字符）就
堆分配——分配时机不可控（拷贝/拼接随时可能触发），且分配路径有锁有抖动。
热路径的用法：协议字段解析全部产出 `string_view`（指向接收缓冲，零拷贝），
需要持久化才在**冷路径**转成 string——
"view 进热路径，owning 留冷路径"是字符串处理的延迟纪律（→ item28 句柄管理）。

</details>
