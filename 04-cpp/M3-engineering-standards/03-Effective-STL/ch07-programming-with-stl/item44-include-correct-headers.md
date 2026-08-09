# Item 44：包含正确的头文件

> 第 7 章 使用 STL 编程 · Item 44 · 上一节：[ch06 仿函数](../ch06-functors-and-functions/README.md) · 下一节：[Item 45 typedef 简化](item45-typedef-simplify.md)

## 为什么要学这个（先建立直觉）

在 C 里，你只 `#include <stdio.h>` 就能用 `printf`。头文件依赖关系简单。C++ STL 不同——组件分散在几十个头文件里，漏含某个头文件在某些编译器上"碰巧能编译"（因为其他头文件间接包含了它），换编译器就报错。

```c
/* C: 一个 stdio.h 搞定 IO */
#include <stdio.h>
int main() { printf("hello\n"); return 0; }
```

```cpp
// C++: 每个组件都有自己的头文件
#include <vector>    // std::vector
#include <algorithm> // std::sort, std::find
#include <numeric>   // std::accumulate（不在 <algorithm> 里！）
#include <string>    // std::string
#include <functional> // std::function, std::bind

int main() {
    std::vector<int> v = {3, 1, 4};
    std::sort(v.begin(), v.end());       // 需要 <algorithm>
    int sum = std::accumulate(v.begin(), v.end(), 0); // 需要 <numeric>
    return 0;
}
```

**直觉**：STL 的头文件按"组件类别"拆分，不是一个大而全的头。显式包含你用到的每个头文件，代码才可移植。

## 这节讲什么

### 常用头文件速查表

| 用途 | 头文件 | 典型组件 |
|------|--------|----------|
| 顺序容器 | `<vector>` `<list>` `<deque>` `<array>` | vector, list, deque, array |
| 关联容器 | `<map>` `<set>` | map, set, multimap, multiset |
| 无序容器 | `<unordered_map>` `<unordered_set>` | unordered_map/set |
| 适配器 | `<stack>` `<queue>` | stack, queue, priority_queue |
| 字符串 | `<string>` | string, wstring |
| 算法 | `<algorithm>` | sort, find, copy, transform, for_each |
| 数值算法 | `<numeric>` | accumulate, iota, partial_sum |
| 迭代器 | `<iterator>` | iterator_traits, back_inserter, stream_iterator |
| 函数对象 | `<functional>` | function, bind, hash, less, greater |
| IO | `<iostream>` `<fstream>` `<sstream>` | cin/cout, fstream, istringstream |
| 工具 | `<utility>` | pair, move, forward, swap |
| 内存 | `<memory>` | unique_ptr, shared_ptr, make_unique/shared |
| 类型 | `<type_traits>` | is_integral, enable_if, conditional |
| 数学 | `<cmath>` | sqrt, abs, pow |
| 时间 | `<chrono>` | duration, time_point, high_resolution_clock |

### 间接包含的陷阱

```cpp
// 某些实现中 <iostream> 间接包含了 <vector> 和 <string>
#include <iostream>
// 所以下面的代码"碰巧"能编译：
std::vector<int> v;        // 间接包含 <vector>
std::string s = "hello";   // 间接包含 <string>

// 但换到另一个编译器（或新版），间接包含关系可能变化
// → 编译失败：'vector' is not a member of 'std'
```

**规则**：你用到的每个 STL 组件，都显式 `#include` 它的头文件。不要依赖间接包含。

### C++ 标准头文件 vs C 头文件

```cpp
// C++ 推荐用 <cXXX> 形式（组件在 std:: 命名空间）
#include <cstdio>    // std::printf, std::fopen
#include <cstdlib>   // std::malloc, std::atoi
#include <cstring>   // std::strlen, std::memcpy

// C 风格 <XXX.h> 也能用，但已废弃
#include <stdio.h>   // printf, fopen（全局命名空间）
#include <string.h>  // strlen, memcpy
```

| C++ 头文件 | C 头文件 | 内容 |
|-----------|---------|------|
| `<cstdio>` | `<stdio.h>` | printf, fopen, fread |
| `<cstdlib>` | `<stdlib.h>` | malloc, exit, atoi |
| `<cstring>` | `<string.h>` | strlen, memcpy, strcpy |
| `<cmath>` | `<math.h>` | sqrt, sin, abs |
| `<cassert>` | `<assert.h>` | assert |

## 常见错误（新手踩坑）

### 错误 1：漏含 `<numeric>`

```cpp
#include <algorithm>
#include <vector>
// int sum = std::accumulate(v.begin(), v.end(), 0);  // 某些编译器报错
// accumulate 在 <numeric> 里，不在 <algorithm>！
```

**修复**：加 `#include <numeric>`。

### 错误 2：漏含 `<functional>`

```cpp
#include <vector>
#include <algorithm>
// std::sort(v.begin(), v.end(), std::greater<int>());  // 报错
// std::greater 在 <functional> 里
```

**修复**：加 `#include <functional>`。

### 错误 3：用 C 风格 `.h` 头文件

```cpp
#include <stdio.h>    // 能用，但不推荐
#include <iostream>   // C++ 风格

// 混用 C 风格和 C++ 风格不一致
```

**修复**：C++ 代码统一用 `<cXXX>` 形式。

## 新手要点（和 C 的区别）

| 方面 | C | C++ |
|------|---|-----|
| 头文件数 | 少（stdio/stdlib/string 基本够） | 多（按组件拆分） |
| 命名空间 | 全局 | `std::` |
| C 头文件 | `<stdio.h>` | `<cstdio>`（推荐） |
| 间接包含 | 少见 | 常见（但不可依赖） |

## HFT 关联

- **`<chrono>` 精确计时**：HFT 延迟测量用 `std::chrono::high_resolution_clock`，必须 `#include <chrono>`
- **`<algorithm>` 热路径**：sort/find/copy 等是 HFT 常用算法，显式包含
- **`<memory>` 智能指针**：RAII 管理资源，`#include <memory>`

## 代码自测

### Q1: accumulate 头文件

```cpp
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> v = {1, 2, 3};
    int sum = std::accumulate(v.begin(), v.end(), 0);
    return 0;
}
```
> 这段代码在所有编译器上都能编译吗？

<details>
<summary>答案</summary>

**不能保证**。`std::accumulate` 在 `<numeric>` 中，不在 `<algorithm>` 中。

某些实现中 `<algorithm>` 可能间接包含 `<numeric>`，但不能依赖这一点。

**修复**：加 `#include <numeric>`。
</details>

### Q2: C vs C++ 头文件

```cpp
#include <string.h>     // A
#include <cstring>      // B
#include <string>       // C
```
> A、B、C 分别提供什么？

<details>
<summary>答案</summary>

- **A `<string.h>`**：C 头文件，提供 `strlen`/`memcpy`/`strcpy` 等 C 字符串操作（全局命名空间）
- **B `<cstring>`**：C++ 版 C 头文件，同样提供 `strlen`/`memcpy` 等，但在 `std::` 命名空间
- **C `<string>`**：C++ 头文件，提供 `std::string` 类（完全不同的东西！）

**常见混淆**：`<string>` 和 `<cstring>` 不是一回事。`<string>` 是 C++ string 类，`<cstring>` 是 C 字符串函数的 C++ 包装。
</details>

### Q3: 间接包含

```cpp
#include <iostream>

int main() {
    std::vector<int> v;  // 在 GCC 上能编译，在 Clang 上报错
    return 0;
}
```
> 为什么不同编译器行为不同？

<details>
<summary>答案</summary>

`<iostream>` 的实现中可能间接包含了 `<vector>`（GCC 的 libstdc++ 确实如此），但这不是标准保证的。

不同编译器/标准库实现有不同的内部包含关系：
- GCC libstdc++：`<iostream>` → 间接包含 `<vector>`
- Clang libc++：`<iostream>` → 可能不包含 `<vector>`

**规则**：永远显式包含你用到的头文件，不依赖间接包含。代码才可移植。
</details>

### Q4: 完整头文件

```cpp
// 这段代码需要哪些头文件？
std::vector<std::string> lines;
std::ifstream f("data.txt");
std::string line;
while (std::getline(f, line)) {
    lines.push_back(line);
}
std::sort(lines.begin(), lines.end());
auto it = std::find(lines.begin(), lines.end(), "target");
```

<details>
<summary>答案</summary>

需要：
- `#include <vector>` — std::vector
- `#include <string>` — std::string, std::getline
- `#include <fstream>` — std::ifstream
- `#include <algorithm>` — std::sort, std::find

`std::getline` 定义在 `<string>` 中（不是 `<iostream>`！）。
</details>

## 参考与延伸

- 上一节：[ch06 仿函数](../ch06-functors-and-functions/README.md)
- 下一节：[Item 45 typedef 简化](item45-typedef-simplify.md)
