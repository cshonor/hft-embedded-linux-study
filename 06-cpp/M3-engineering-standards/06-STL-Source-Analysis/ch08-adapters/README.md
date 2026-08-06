# 第 8 章 适配器

**Adapters**

## 本章讲什么

适配器（adapter）模式：用一个组件"包装"另一个组件，改造其接口。STL 三类适配器——容器适配器、迭代器适配器、函数适配器——让已有组件以新接口复用。本章讲它们的源码包装机制。

## 要点

### 容器适配器

`stack`/`queue`/`priority_queue` 包装 `deque`（默认）或 `list`/`vector`，只露裁剪后的接口（`push`/`pop`/`top`），无迭代器。`priority_queue` 用堆算法维护优先级。

### 迭代器适配器

| 适配器 | 包装效果 |
|--------|----------|
| `reverse_iterator` | 反向遍历（`rbegin`/`rend`） |
| `back_insert_iterator` | `*it = x` 变 `push_back(x)` |
| `front_insert_iterator` | 变 `push_front(x)` |
| `insert_iterator` | 变 `insert(pos, x)` |
| `istream_iterator`/`ostream_iterator` | 流当容器 |

`back_inserter(c)` 是最常用的——让 `copy` 往目标容器尾插（自动扩容）。

### 函数适配器

`bind1st`/`bind2nd` 绑定参数，`not1`/`not2` 取反，`mem_fun`/`mem_fun_ref` 包装成员函数。C++11 后 `std::bind` + lambda 全面取代。

## HFT 关联

- **`back_inserter` 自动扩容**：`copy(src.begin(), src.end(), back_inserter(dst))` 省去手动 reserve + 下标，但每元素 `push_back` 可能多次扩容——HFT 预 `reserve` 后用 `dst.insert(dst.end(), ...)` 区间版更高效。
- **`priority_queue` 堆**：事件调度（如定时撤单）用 `priority_queue`（默认 `vector` + `less` 堆），但注意它无迭代器、不支持随机删除——HFT 常自建可删除的堆。

## 自测题

1. 容器适配器 `stack` 默认底层是什么？为什么无迭代器？
2. `back_inserter(c)` 让 `*it = x` 变成什么操作？
3. C++11 后什么取代了 `bind2nd`/`mem_fun` 这套函数适配器？
4. `priority_queue` 为什么不支持随机删除？HFT 事件调度如何绕过？
