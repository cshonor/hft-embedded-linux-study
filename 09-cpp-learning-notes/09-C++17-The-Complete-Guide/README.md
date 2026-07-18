# 《C++17 - The Complete Guide》章节索引

> Nicolai Josuttis 著，与 [10-C++20-The-Complete-Guide](../10-C++20-The-Complete-Guide/) 同作者。**08 并发 → 09 C++17 → 10 C++20** 按版本递进，C++17 是 HFT 技术栈里大量「现代特性」的过渡基线。

## 为什么插在 08 与 09 之间

| C++17 特性 | HFT / 低延迟场景 |
|------------|------------------|
| 结构化绑定 | 解包 tick / order 字段，少写临时变量 |
| 折叠表达式 | 可变参模板、编译期聚合日志/校验 |
| `if constexpr` | 热路径零开销分支，替代 SFINAE |
| `string_view` | 零拷贝解析 FIX/CSV 字段 |
| 并行 STL | 离线回测、批处理行情（见 [08 ch10](../08-Cpp-Concurrency/ch10-parallel-algorithms/)） |
| `optional` / `variant` | 可空字段、多态消息体而不必继承 |
| PMR / `to_chars` | 可控分配、无 iostream 的快速数值格式化 |

许多特性在 **C++20 中继续完善**（如 Concepts 约束 CTAD、`jthread` 扩展并发模型、Ranges 延续 `string_view` 思路），先吃透 17 再读 20 更顺。

## 全书结构（五部分）

| 部分 | 章 | 主题 |
|------|-----|------|
| **I** | 1–8 | 基础语言特性 |
| **II** | 9–14 | 模板特性 |
| **III** | 15–20 | 新库组件 |
| **IV** | 21–28 | 库扩展与修改 |
| **V–VI** | 29–35 | 专家工具、编译设置、弃用移除 |

## 章节目录

| 章 | 目录 | 英文标题 |
|----|------|----------|
| 01 | [第 1 章 结构化绑定](./ch01-structured-bindings/) | Structured Bindings |
| 02 | [第 2 章 if/switch 带初始化](./ch02-if-switch-init/) | if and switch with Initialization |
| 03 | [第 3 章 内联变量](./ch03-inline-variables/) | Inline Variables |
| 04 | [第 4 章 聚合类型扩展](./ch04-aggregate-extensions/) | Aggregate Extensions |
| 05 | [第 5 章 强制拷贝省略与未物化传值](./ch05-copy-elision/) | Mandatory Copy Elision or Passing Unmaterialized Objects |
| 06 | [第 6 章 Lambda 扩展](./ch06-lambda-extensions/) | Lambda Extensions |
| 07 | [第 7 章 新属性与属性扩展](./ch07-attributes/) | New Attributes and Attribute Features |
| 08 | [第 8 章 其他语言特性](./ch08-other-language-features/) | Other Language Features |
| 09 | [第 9 章 类模板参数推导 CTAD](./ch09-ctad/) | Class Template Argument Deduction |
| 10 | [第 10 章 编译期 if](./ch10-constexpr-if/) | Compile-Time if |
| 11 | [第 11 章 折叠表达式](./ch11-fold-expressions/) | Fold Expressions |
| 12 | [第 12 章 字符串字面量作模板参数](./ch12-string-literals-nttp/) | Dealing with String Literals as Template Parameters |
| 13 | [第 13 章 auto 作模板参数](./ch13-auto-template-params/) | Placeholder Types like auto as Template Parameters |
| 14 | [第 14 章 扩展 using 声明](./ch14-extended-using/) | Extended Using Declarations |
| 15 | [第 15 章 std::optional](./ch15-optional/) | std::optional<> |
| 16 | [第 16 章 std::variant](./ch16-variant/) | std::variant<> |
| 17 | [第 17 章 std::any](./ch17-any/) | std::any |
| 18 | [第 18 章 std::byte](./ch18-byte/) | std::byte |
| 19 | [第 19 章 字符串视图](./ch19-string-view/) | String Views |
| 20 | [第 20 章 文件系统库](./ch20-filesystem/) | The Filesystem Library |
| 21 | [第 21 章 type_traits 扩展](./ch21-type-traits/) | Extensions of Type Traits |
| 22 | [第 22 章 并行 STL 算法](./ch22-parallel-stl/) | Parallel STL Algorithms |
| 23 | [第 23 章 新 STL 算法详解](./ch23-new-stl-algorithms/) | New STL Algorithms in Detail |
| 24 | [第 24 章 子串与子序列搜索器](./ch24-subsequence-searchers/) | Substring and Subsequence Searchers |
| 25 | [第 25 章 其他工具函数与算法](./ch25-utility-algorithms/) | Other Utility Functions and Algorithms |
| 26 | [第 26 章 容器与 string 扩展](./ch26-container-extensions/) | Container and String Extensions |
| 27 | [第 27 章 多线程与并发库](./ch27-multithreading/) | Multi-Threading and Concurrency |
| 28 | [第 28 章 其他标准库小改进](./ch28-small-library-features/) | Other Small Library Features and Modifications |
| 29 | [第 29 章 多态内存资源 PMR](./ch29-pmr/) | Polymorphic Memory Resources (PMR) |
| 30 | [第 30 章 超对齐 new/delete](./ch30-over-aligned-new/) | new and delete with Over-Aligned Data |
| 31 | [第 31 章 to_chars / from_chars](./ch31-to-from-chars/) | std::to_chars() and std::from_chars() |
| 32 | [第 32 章 std::launder](./ch32-launder/) | std::launder() |
| 33 | [第 33 章 泛型代码实现改进](./ch33-generic-code-improvements/) | Improvements for Implementing Generic Code |
| 34 | [第 34 章 常用 C++17 编译设置](./ch34-cpp17-settings/) | Common C++17 Settings |
| 35 | [第 35 章 弃用与移除特性](./ch35-deprecated-removed/) | Deprecated and Removed Features |

## 其他组成部分

- [前言 Preface](./preface/)

## 推荐阅读顺序

1. **语言层速通**：1（结构化绑定）→ 2 → 6 → 9–11（CTAD / `if constexpr` / 折叠表达式）
2. **HFT 高频库**：19（`string_view`）→ 22（并行 STL，配合 08）→ 31（`to_chars`）→ 29（PMR）
3. **类型安全与配置**：15–17（`optional` / `variant` / `any`）→ 20（filesystem 配置/日志路径）
4. **收尾**：34（`-std=c++17` 等设置）→ 35（弃用特性）→ 进入 [10 C++20](../10-C++20-The-Complete-Guide/)

## 学习进度

- [ ] 第 1 章
- [ ] 第 2 章
- [ ] 第 3 章
- [ ] 第 4 章
- [ ] 第 5 章
- [ ] 第 6 章
- [ ] 第 7 章
- [ ] 第 8 章
- [ ] 第 9 章
- [ ] 第 10 章
- [ ] 第 11 章
- [ ] 第 12 章
- [ ] 第 13 章
- [ ] 第 14 章
- [ ] 第 15 章
- [ ] 第 16 章
- [ ] 第 17 章
- [ ] 第 18 章
- [ ] 第 19 章
- [ ] 第 20 章
- [ ] 第 21 章
- [ ] 第 22 章
- [ ] 第 23 章
- [ ] 第 24 章
- [ ] 第 25 章
- [ ] 第 26 章
- [ ] 第 27 章
- [ ] 第 28 章
- [ ] 第 29 章
- [ ] 第 30 章
- [ ] 第 31 章
- [ ] 第 32 章
- [ ] 第 33 章
- [ ] 第 34 章
- [ ] 第 35 章
- [ ] 前言
