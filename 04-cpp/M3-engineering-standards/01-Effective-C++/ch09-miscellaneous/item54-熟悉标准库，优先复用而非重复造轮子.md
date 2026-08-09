# 条款 54：熟悉标准库，优先复用而非重复造轮子

## 本节讲什么

STL 容器、算法、智能指针、字符串等工业级实现，稳定高效，自己手写极易踩坑。

## 示例

```cpp
#include <algorithm>
#include <vector>
std::sort(v.begin(), v.end());  // 优先标准库
```

---

## 代码自测

**题目 1：** 以下功能 C++ 标准库已经提供，不该手写。写出对应的头文件。
1) 动态数组 → ?
2) 链表 → ?
3) 哈希表 → ?
4) 智能指针 → ?
5) 正则表达式 → ?
6) 线程 → ?

<details>
<summary>参考答案</summary>

1) `std::vector` — `<vector>`
2) `std::list` — `<list>`
3) `std::unordered_map` — `<unordered_map>`
4) `std::unique_ptr` / `std::shared_ptr` — `<memory>`
5) `std::regex` — `<regex>`
6) `std::thread` — `<thread>`
标准库经过充分测试、跨平台、性能优化。优先复用而非重复造轮子。

</details>
