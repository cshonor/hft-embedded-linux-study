# 条款 44：将与参数无关的代码抽离 templates

## 本节讲什么

**Factor parameter-independent code out of templates.** 模板为每个类型参数
实例化一份代码——**与参数无关的逻辑被原样复制 N 份**，这是模板代码膨胀
（code bloat）的主要来源。抽离手法：参数无关部分下沉到**非模板基类/函数**，
模板只做薄薄一层类型包装。

← 上一条 [item43 模板基类名称查找](./item43-学习处理模板化基类内的名称.md)；
下一条 [item45 成员函数模板](./item45-运用成员函数模板接受所有兼容类型.md)。

---

## 1. 问题：膨胀是怎么发生的

```cpp
template <typename T, std::size_t N>
class FixedQueue {
    T data_[N];
    std::size_t head_ = 0, tail_ = 0;
public:
    bool push(const T& x) {
        if (full()) return false;        // full() 逻辑与 T 完全无关
        data_[tail_] = x;                // 只有这一行真正依赖 T
        tail_ = (tail_ + 1) % N;         // 下标运算与 T 无关
        return true;
    }
    // empty()/full()/size()/下标回绕——全部与 T 无关，却随每个 T 实例化一份
};
// FixedQueue<int>、FixedQueue<Tick>、FixedQueue<Order>——
// full()/empty()/下标逻辑的机器码被复制三份
```

模板实例化的规则是**整个类来一份**——哪怕某个成员函数一个 T 都没用到，
它照样被复制（除非编译器/链接器做 ICF 折叠，别赌这个）。

## 2. 抽离手法一：非模板基类承载参数无关逻辑

```cpp
class QueueBase {                        // 非模板：全程序只此一份
protected:
    std::size_t head_ = 0, tail_ = 0, cap_;
    explicit QueueBase(std::size_t cap) : cap_(cap) {}
    bool full() const { return (tail_ + 1) % cap_ == head_; }   // 与 T 无关的逻辑全在这
    bool empty() const { return head_ == tail_; }
    std::size_t advance(std::size_t i) const { return (i + 1) % cap_; }
};

template <typename T, std::size_t N>
class FixedQueue : private QueueBase {   // 私有继承（item39）：要实现不要接口
public:
    FixedQueue() : QueueBase(N) {}
    bool push(const T& x) {
        if (full()) return false;        // 复用基类——零复制
        storage()[tail_] = x;            // 模板层只做与 T 有关的事
        tail_ = advance(tail_);
        return true;
    }
private:
    T storage_[N];
    T* storage() { return storage_; }
};
```

模板类变成**薄薄的类型壳**：实例化 N 份，每份只剩与 T 真正相关的几行；
队列逻辑（判满/判空/下标回绕）全程序只有一份。

## 3. 抽离手法二：指针/void* 抹平类型差异

```cpp
template <typename T>
class Stack {
    std::vector<void*> slots_;           // 存储层统一用 void*——一份代码
public:
    void push(T* p) { slots_.push_back(p); }      // inline 薄壳
    T* pop() { auto* p = static_cast<T*>(slots_.back()); slots_.pop_back(); return p; }
};
// Stack<int>、Stack<Order> 的 push/pop 机器码相同——链接器 ICF 大概率折叠；
// 想更彻底：存储层抽成非模板的 PtrStack，模板只做类型安全包装
```

STL 的 `vector<T*>` 在主流实现里就复用 `vector<void*>` 的代码——
指针类型实例化的膨胀被实现者吃掉了。

## 4. 判别与边界

**该抽的信号**：读模板类时，把"不含 T 的成员函数/逻辑"圈出来——
圈出的部分超过三成，就该考虑下沉。

**别抽的情况**：
- 逻辑虽不含 T，但**依赖布局**（`sizeof(T)`、对齐、数组步长）——
  这些"参数无关"是假象，实例化后行为真的不同
- 抽离要付出**间接层**（基类指针/void* 转换）且类型在热路径——
  膨胀换性能是热路径的合理交易（→ 与 item30 I-cache 的取舍平衡：
  膨胀伤 I-cache，间接伤数据局部性，两个都测过再选）

## HFT 关联

- **无锁队列/环形缓冲**是抽离的天然候选：序号回绕、缓存行对齐、
  内存序操作全部与元素类型无关——下沉到非模板基类，
  `SpscRing<T>` 只剩"在哪 placement new"（→ 19.1）
- 模板膨胀在交易系统里是真实成本：几十个协议消息类型 × 完整解码模板 =
  I-cache 压力（32K L1i 很容易爆）——"薄模板壳 + 厚非模板基类"是
  协议解码库的标准结构
- 别在**类型决定布局**的地方抽：订单簿档位按 T 定宽（Price4/Price8），
  步长不同——这种"看似无关"的逻辑抽出去就是 bug

## 代码自测

**题目 1：** 模板代码膨胀的根源是什么？为什么"成员函数没用到 T"也会被复制？

<details>
<summary>参考答案</summary>

根源：模板实例化以**整个类**为单位——`FixedQueue<int>` 实例化时，
类的所有成员（包括完全不含 int 的 `full()`/`empty()`）都生成一份机器码。
编译器没有义务替你分析"这个成员函数与参数无关"；
链接器的 ICF（Identical Code Folding）能事后折叠相同机器码，
但那是优化不是保证（不同 T 的调试信息/重定位可能阻止折叠）。
唯一可靠的手段是**设计时抽离**：参数无关逻辑下沉到非模板基类/函数，
模板只剩类型薄壳。

</details>

**题目 2：** 抽离参数无关代码的两个手法，各自牺牲了什么？

<details>
<summary>参考答案</summary>

① **非模板基类**：牺牲一层继承关系（通常私有继承，item39）和一点间接——
基类函数不再是模板上下文（极端情况下失去按 T 内联特化的机会）；
换来逻辑全程序一份。
② **void*/指针抹平**：牺牲类型安全边界（`static_cast` 集中在薄壳层，
错了是 reinterpret 级别的风险）和一次指针间接；
换来指针族实例（`Stack<int*>`/`Stack<Order*>`）共享同一份代码。
共同原则：抽离层**必须薄**（inline 包装），类型安全在薄壳里一次检完——
不让 void* 漏到模板层之外。

</details>

**题目 3：** 热路径的 `OrderBook<Price4>` 和 `OrderBook<Price8>`，
"档位比较逻辑"（与价格类型无关）该不该抽离到非模板基类？

<details>
<summary>参考答案</summary>

**不该**——这是"参数无关"的假象：Price4 和 Price8 的**布局/对齐/步长不同**，
比较逻辑生成的机器码（加载宽度、对齐假设）实例化后真的不同；
抽成非模板基类就得经指针间接 + 失去按布局特化的内联机会，
热路径性能反而受损。
判别：逻辑"与 T 无关"指的是**语义**无关还是**机器码**无关？
本例语义无关但机器码相关——不抽。
真正该抽的是序号回绕、容量计算、内存序协议这类
语义和机器码都与 T 无关的部分（→ 无锁队列的抽离实践）。

</details>
