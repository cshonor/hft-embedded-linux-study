# 条款 54：熟悉标准库，优先复用而非重复造轮子

## 本节讲什么

STL 容器、算法、智能指针、字符串等工业级实现，稳定高效，自己手写极易踩坑。

## 示例

```cpp
#include <algorithm>
#include <vector>
std::sort(v.begin(), v.end());  // 优先标准库
```
