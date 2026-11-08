# Item 17a：避免使用 `vector<bool>`

> 第 2 章 vector 和 string · Item 17a（原书 Item 18，补遗） · 上一节：[Item 17 交错使用 vector 和 string 数据](item17-vector-string-interop.md)

## 为什么要学这个（先建立直觉）

`vector<bool>` 看起来是「存 bool 的 vector」——名字里带着 vector，能用就完了？
它实际上是**标准库里最出名的「名不副实」**：

```cpp
std::vector<bool> flags = {true, false, true};

bool* p = &flags[0];        // ❌ 编译错误！元素地址都取不了
bool& r = flags[0];         // ❌ 编译错误！返回的不是 bool&

auto x = flags[0];          // ✅ 能编译……但 x 不是 bool，是个代理对象
```

它不是容器，是**打包存储的位压缩代理**——Meyers 的建议就两个字：别用。

## 这节讲什么

`vector<bool>` 是 `vector` 的一个**特化**（不是真正的容器模板实例化结果）：
为了省空间，它把每个 bool 压缩成 **1 个 bit**，64 个 bool 挤在一个 `uint64_t` 里。

代价是毁灭性的——**单个 bit 没有地址**，而 C++ 的引用和指针都必须指向可寻址对象。
于是标准委员会造了个 workaround：让 `operator[]` 和迭代器解引用返回一个
**代理对象 `std::vector<bool>::reference`**，内部存「字地址 + 位偏移」，用转换运算符
模拟 `bool` 的行为。

## 代理对象引发的连锁反应

```cpp
std::vector<bool> v = {true, false};

// 1. 不满足容器要求：T& operator[](size_type)
v[0] = false;                // 能写？——可以，代理重载了 operator=
bool* p = &v[0];             // ❌ 取不了地址：代理是临时对象

// 2. 泛型代码直接碎
template<typename C>
void flip_first(C& c) { auto& e = c[0]; e = !e; }
// 对 vector<bool> 编译失败：auto& 绑不上右值代理

// 3. auto 推导出代理类型（C++11 经典暗坑）
auto x = v[0];               // x 的类型是 vector<bool>::reference，不是 bool！
v.clear();
bool b = x;                  // UB：代理悬垂，指向已销毁的存储
```

标准对「容器」的要求包括 `operator[]` 返回 `T&`、迭代器解引用返回 `T&`——
`vector<bool>` 全都做不到，所以**它连 STL 容器都不算**，只是顶着 vector 的名字。

## 为什么它还存在

历史包袱：90 年代标准委员会想试验「空间优化的代理容器」，`vector<bool>` 是试验品。
后来共识是**这是设计错误**，但 ABI 兼容让它删不掉。
教训本身值得记住：**「省内存」的优化一旦破坏接口契约，省下的 bit 远不够付债。**

## 替代方案（按场景选）

| 需求 | 替代 | 说明 |
|------|------|------|
| 通用 bool 序列 | `std::deque<bool>` | 真容器，元素真 bool，可寻址 |
| 标志位数组（性能优先） | `std::vector<uint8_t>` / `vector<char>` | 1 字节/标志，cache 友好，推荐 |
| 固定大小位集 | `std::bitset<N>` | 编译期定长，真位压缩，但接口就是位接口（名实相符） |
| 动态位集 | `boost::dynamic_bitset` | 真位压缩 + 明确位语义，不伪装成容器 |

关键区分：`vector<uint8_t>` 和 `bitset` 都是**名实相符**的——
一个是字节数组，一个是位集。`vector<bool>` 的病在于**用容器的名字干位集的事**。

## HFT 关联

- **热路径标志位用 `uint8_t` 数组而不是位压缩**：1 bit/标志省下 7/8 内存，
  但每次访问多一次掩码运算 + 多一层代理间接；更重要的是位压缩让相邻标志
  **共享同一个字**——多线程写相邻标志直接 false sharing 变真数据竞争。
  `uint8_t` 一字节一标志，配合 padding 隔离，才是 HFT 的正确姿势。
- **订单状态机、连接状态表**：固定规模用 `bitset<N>`（明确位语义），
  动态规模用 `vector<uint8_t>`——两个都不踩代理坑。
- **面试/代码评审信号**：看到 `vector<bool>` 就追问一句「知道它是代理吗」——
  能答上来的基本把 STL 容器契约搞明白了。

## 代码自测

### Q1：这段代码输出什么？为什么？

```cpp
std::vector<bool> v = {true};
auto x = v[0];
v[0] = false;
std::cout << x;
```

<details>
<summary>答案与复习指引</summary>

输出 **0**（false）——不是很多人直觉的 1。

`auto x = v[0];` 推导出的不是 `bool`，是代理对象 `vector<bool>::reference`，
它内部存着「字地址 + 位偏移」，**像引用一样指向容器内部**。
`v[0] = false` 之后，`x` 再转 bool 读到的就是新值。

这正是代理对象的危险：你以为拷了一个值，实际拿了一个隐式引用。
对比：`std::vector<int> vi = {1}; auto y = vi[0];` 里 `y` 是真 `int` 副本。

复习：`auto` + 代理类型 = 类型推导暗坑（Effective Modern C++ Item 6 同款）。
</details>

### Q2：`vector<bool>` 省空间，为什么 HFT 反而不该用？

<details>
<summary>答案与复习指引</summary>

三个原因：

1. **访问成本**：每次读写都是「定位字 → 移位 → 掩码」三连，比直接按字节访问贵；
2. **cache 语义反转**：位压缩让逻辑上相邻的标志物理上**共享一个字**。
   单线程省 cache，多线程时两个线程写「不同」标志其实在写同一个字——
   从 false sharing 升级为真数据竞争，连「不同元素是不同对象」这条底线都没了；
3. **代理间接层**：`operator[]` 返回代理，编译器优化障碍，还给泛型代码埋雷。

HFT 的空间哲学：内存便宜，确定性贵。`vector<uint8_t>` + padding 才是正道。

复习：false sharing 见 CCiA ch08；「名实相符」原则。
</details>

### Q3：为什么 `std::bitset<N>` 也是位压缩，却不算「坑」？

<details>
<summary>答案与复习指引</summary>

**名实相符**。`bitset` 的名字、文档、接口全都明说「我是位集」：
没有 `operator[]` 返回引用（返回的也是代理，但没人指望它满足容器契约）、
不提供迭代器、不要求可寻址。你用它的时候知道自己用的是位集。

`vector<bool>` 的病在于**伪装**：顶着容器的名字，不满足容器的契约，
专杀那些按「它是容器」写泛型代码的人。

设计启示：优化实现可以，改变契约不行——改了契约就该改名字。

复习：STL 容器契约（`operator[]` 返回 `T&`、元素可寻址）。
</details>

## 延伸

- 原书：Effective STL Item 18
- 进阶：Effective Modern C++ Item 6（`auto` 推导到代理类型时的隐形坑，用
  `static_cast` 显式指定：`auto x = static_cast<bool>(v[0]);`）
- 相关：Item 12-13（vector 容量与扩容）——真 vector 的内存语义；
  boost::dynamic_bitset（真需要动态位集时）
