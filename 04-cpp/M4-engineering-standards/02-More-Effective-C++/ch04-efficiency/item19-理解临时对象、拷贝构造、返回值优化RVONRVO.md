# 条款 19：临时对象、拷贝构造与 RVO/NRVO（综合复习）

## 本节讲什么

**Master temporaries, copying, and RVO/NRVO.** 本条是效率章的枢纽——
把 item16（临时对象产地）、Effective item21（RVO 实测）、19.7（移动语义）
串成一条完整的成本链：**对象在你看不见的地方被拷贝了多少次，
以及现代 C++ 如何让这些拷贝消失**。

← 上一条 [item18 自定义内存池](./item18-通过重载operatornew实现自定义内存池，减少频繁堆分配损耗.md)；
下一条 [item20 静态 vs 动态绑定](./item20-按需选用静态绑定动态绑定（虚函数），不要无脑虚函数增加开销.md)。

---

## 1. 成本链全景（三个时代的同一段代码）

```cpp
Rational operator*(const Rational& a, const Rational& b) { return Rational(...); }
Rational r = a * b;
```

| 时代 | 实际发生 | 成本 |
|---|---|---|
| C++98（无 RVO 保证） | 局部构造 → 拷贝到临时 → 拷贝到 r（2 次拷贝 + 3 次析构） | 高 |
| C++11/14（RVO 优化 + 移动） | 局部构造 → 移动到 r（编译器几乎总做 NRVO） | 低 |
| C++17+（RVO **强制**） | 直接在 r 的存储上构造（→ Effective item21 本机实测：-O0 也零拷贝） | **零** |

**推论**：现代 C++ 里"按值返回"已经零成本——
还在为了"省拷贝"返回引用/输出参数的代码，多数是在跟 C++98 的幽灵作战
（Effective item21 的完整论证 + 实测）。

## 2. 拷贝的真实残留地（现代 C++ 里拷贝躲在哪）

RVO 消不掉的三类拷贝（效率优化的真实战场）：

```cpp
// ① 函数参数的按值传递
void process(std::string s);        // 调用处必拷（除非实参是右值 → 移动）
process(name);                      // name 是左值：拷贝构造一次

// ② 容器扩容的元素搬迁
vec.push_back(x);                   // 扩容时 N 个元素移动（或拷贝——noexcept 决定，
                                    // → 18.1 ② 实测：移动没 noexcept 就退化为拷贝）

// ③ 你亲手写的拷贝
auto backup = current_config;       // copy-and-swap 的第一步（Effective item29：
                                    // 这是有意付费的拷贝——强保证的成本）
```

判别：这个拷贝是**意外**（写法问题，能消）还是**代价**（语义需要，该付）？
item16 管意外，本条管代价的定价。

## 3. 移动语义的边界（别把 move 当万能）

- `std::move` 不移动（→ 19.7 ④ 实测：它只是类型转换，移动发生在接收方）
- 移动后对象"有效但未指定"（19.7 实测 size()=0 是 libstdc++ 细节不是标准承诺）
- **小对象移动可能不省**：POD ≤ 16B 的移动和拷贝成本相同（都是寄存器搬运）——
  move 的收益在"带资源的对象"（string/vector/unique_ptr 的指针过户）
- 移动构造必须 noexcept 才被容器信任（18.1 ②）——标 noexcept 是性能承诺

## HFT 关联

- 热路径的拷贝审计只需盯②和③：容器扩容（预分配 reserve 消灭之）、
  有意拷贝（配置快照 copy-and-swap，冷路径专属）——
  ① 的按值传参在小 POD 上本来就该按值（Effective item20 的例外）
- 订单簿的 `reserve()` 是消灭"扩容搬迁"的标准动作：
  档位上限已知（合约深度），启动时 reserve 到位 = 运行期零搬迁零分配
- `return std::move(local)` 仍是审查清单第一条（Effective item21 ②：
  阻止 NRVO 的反优化——本机实测 NRVO 在 -O0 都生效）

## 代码自测

**题目 1：** C++17 起 `Rational r = a * b;` 的真实成本是什么？为什么 -O0 也一样？

<details>
<summary>参考答案</summary>

**一次构造，零拷贝零移动**——C++17 把 RVO（return 纯右值表达式）从
"优化"升级为**语言保证**：`return Rational(...)` 的构造直接发生在
r 的存储上（Effective item21 本机实测：g++ -O0 输出只有"构造"，无拷贝无移动）。
-O0 也生效因为它是**语义**不是优化——编译器无权不做
（这与 NRVO（具名局部变量）不同：NRVO 仍属"允许但不强制"，
主流编译器 -O0 也做，但标准不保证）。
结论：现代 C++ 按值返回是零成本惯用法，为它返回引用是在跟 C++98 的幽灵作战。

</details>

**题目 2：** `vec.push_back(x)` 的"扩容搬迁"在什么条件下退化为拷贝？怎么验证和修？

<details>
<summary>参考答案</summary>

当元素类型的移动构造**未标 noexcept** 时：vector 扩容要保强保证
（搬一半抛异常还能回滚），不敢用可能抛的移动——退化为拷贝（→ 18.1 ② 本机实测：
MoveThrow 版扩容打印"拷贝"，MoveNoexcept 版打印"移动"）。
验证：给移动/拷贝构造各加打印，`reserve` 触发扩容观察输出；
或 `static_assert(std::is_nothrow_move_constructible_v<T>)` 编译期钉死。
修复：移动构造/移动赋值/swap 全部 noexcept（三件套，
→ item14 的 noexcept 语义）——这是自定义值类型进 STL 容器的入场券。

</details>

**题目 3：** 热路径上 `void process(Price p)` 按值传参是错误吗？

<details>
<summary>参考答案</summary>

**不是**——这是 Effective item20 的例外：POD 小对象（Price 通常 4-8B）
按值传递走**寄存器**，比 const 引用（一次解引用 + 潜在 cache miss）
**更快**。判别规则：
按值 ≤ 16B 的 trivially copyable 类型 → 按值（寄存器直达）；
带资源的/大的 → const 引用；
要"吃掉"参数（sink）→ 按值 + move（移动构造接管，调用方传右值时零拷贝）。
热路径的参数形态审查：`sizeof(T) <= 16 && is_trivially_copyable_v<T>`
的 static_assert 值得写进 Price/Qty 类里——它同时保证
"按值合理"和"可 memcpy"（→ 19.8 守卫模式）。

</details>
