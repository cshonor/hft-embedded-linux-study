# 迭代器与范围概念体系

## 迭代器概念的强弱阶梯

C++20 把迭代器能力按"能做什么"排成严格的偏序，每一档都是在上一档上加要求：

```
input_iterator           能读、能前进一次（单遍，如 istream_iterator）
  └ forward_iterator     能多遍扫（前进后旧的副本仍有效）
      └ bidirectional_iterator   还能 --（list）
          └ random_access_iterator   还能 += n、[]、<=>（vector、原生指针）
              └ contiguous_iterator    且元素在内存中连续（vector、array、span）
```

另有一条正交支路：`output_iterator`（只能写）、`sentinel_for`（哨兵类型可与迭代器比较）。

```cpp
#include <iterator>
#include <vector>
#include <list>
#include <span>

static_assert(std::contiguous_iterator<std::vector<int>::iterator>);
static_assert(std::contiguous_iterator<int*>);                    // 原生指针也是
static_assert(std::random_access_iterator<std::span<int>::iterator>);
static_assert(std::contiguous_iterator<std::span<int>::iterator>);// span 也连续
static_assert(std::bidirectional_iterator<std::list<int>::iterator>);
static_assert(!std::random_access_iterator<std::list<int>::iterator>);
```

## 为什么 contiguous_iterator 对 HFT 是单独一档

`random_access` 只保证 O(1) 跳转，**不保证内存连续**（`deque` 迭代器就是 random_access 但不连续）。而连续内存才能：

```cpp
template <std::contiguous_iterator I>
void send_batch(I first, I last, int fd) {
    // contiguous → &*first 合法且元素紧挨着 → 一次 writev/send 发走
    auto n = static_cast<size_t>(last - first);
    ::write(fd, std::to_address(first), n * sizeof(*first));
    // to_address：从迭代器拿原生指针（对花式迭代器也安全）
}

std::vector<Tick> batch;
send_batch(batch.begin(), batch.end(), fd);        // ✅
std::span<Tick> window = batch;
send_batch(window.begin(), window.end(), fd);      // ✅ 零拷贝窗口
// std::deque<Tick> dq; send_batch(dq.begin(), ...); // ❌ 编译错误，正确！
//   deque 分段存储，真发出去会读到段间缝隙——concept 在编译期救了你
```

这就是 concepts 的实际价值：**把"我能批量 send"这个内存布局要求编进类型系统**。

## 范围概念：给容器整体贴标签

```cpp
#include <ranges>

namespace r = std::ranges;

// 阶梯和迭代器对应：
r::range                    // 有 begin/end
r::input_range              // 单遍
r::forward_range            // 多遍
r::random_access_range
r::contiguous_range         // data() + size() 可用
r::sized_range              // size() O(1)
r::borrowed_range           // 右值传进来迭代器也不悬垂（vector 不是，span 是）
r::view                     // range + 可移动 + 拷贝/移动 O(1)（"只是个视图"）
```

```cpp
static_assert(r::contiguous_range<std::vector<Tick>>);
static_assert(r::contiguous_range<std::span<Tick>>);
static_assert(r::sized_range<std::vector<Tick>>);
static_assert(!r::borrowed_range<std::vector<Tick>>);  // 右值 vector 的迭代器会悬垂
static_assert(r::borrowed_range<std::span<Tick>>);     // span 只借窗口，安全
```

## borrowed_range：ranges 算法悬垂防护的原理

```cpp
auto get_ticks();   // 返回 vector<Tick>（右值）

// ranges::find 返回 borrowed_iterator_t<R>：
//   对非 borrowed_range 的右值 → 返回 dangling 类型，解引用即编译错误
auto it = r::find(get_ticks(), target);
// *it;   // ❌ it 的类型是 ranges::dangling——悬垂被降级成编译错误

std::span<Tick> s = /* 长生命周期缓冲区 */;
auto it2 = r::find(s, target);
// *it2;  // ✅ span 是 borrowed_range，find 返回真迭代器
```

C++17 的 `std::find` 对此完全沉默（返回悬垂迭代器，UB 到运行期才炸）；C++20 用 `borrowed_range` 这个 concept 把同一类 bug 变成类型错误。

## 用概念约束自己的算法

```cpp
// 只接受"可随机访问且元素可比较"的范围 → 里面才敢用二分
template <r::random_access_range R>
    requires std::totally_ordered<r::range_value_t<R>>
auto lower_tick(R&& range, const r::range_value_t<R>& key) {
    return r::lower_bound(range, key);
}

// 常用配套类型萃取：
r::range_value_t<R>       // 元素类型
r::iterator_t<R>          // 迭代器类型
r::range_reference_t<R>   // 元素引用类型（解引用的结果）
std::iter_value_t<I>      // 迭代器指向的值类型
```

## 自测题

1. `random_access_iterator` 和 `contiguous_iterator` 的差别是什么？哪个标准容器是前者但不是后者？为什么这对批量网络发送是致命的？
2. `std::to_address` 相比 `&*it` 多解决了什么问题？
3. `vector` 为什么不是 `borrowed_range` 而 `span` 是？`ranges::find(右值容器, x)` 返回什么类型、防住什么 bug？
4. `sized_range` 只要求什么？为什么它和 `forward_range` 是正交的（举个单遍但有 size 的例子）？
5. 约束 `lower_tick` 时为什么用 `random_access_range` 而不是 `forward_range`？（提示：`lower_bound` 在不同迭代器档下的复杂度）
