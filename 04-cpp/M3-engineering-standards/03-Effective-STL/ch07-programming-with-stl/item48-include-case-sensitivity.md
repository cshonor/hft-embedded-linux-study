# Item 48：`#include` 路径大小写

> 第 7 章 使用 STL 编程 · Item 48 · 上一节：[Item 47 不依赖实现](item47-no-implementation-assumptions.md) · 下一节：[Item 49 解读错误信息](item49-read-error-messages.md)

## 为什么要学这个（先建立直觉）

在 C 里你也遇到过：`#include <StdIO.h>` 在 Windows 上能编译（NTFS 不区分大小写），但在 Linux 上报错（ext4 区分大小写）。C++ 同理，但更容易犯错——因为 C++ 头文件名更多。

```c
/* C: Windows 上碰巧能编译 */
#include <StdIO.h>   /* Windows: OK（NTFS 不区分大小写） */
                     /* Linux:   fatal error: StdIO.h: No such file */
```

```cpp
// C++: 同样的问题
#include <Vector>    // Windows: 可能 OK
                     // Linux:   报错
#include <ALGORITHM> // 任何平台都可能出问题
```

**直觉**：标准 C++ 头文件全部小写。始终用小写，代码才能跨平台编译。

## 这节讲什么

### 标准头文件命名规则

C++ 标准头文件全部使用**小写字母**，无扩展名：

```cpp
// ✅ 正确（全小写）
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <type_traits>
#include <chrono>

// ❌ 错误（大写或混合）
#include <Vector>           // Linux 报错
#include <AlgoRithm>        // 任何平台都可能出问题
#include <unordered_map.h>  // C++ 标准头没有 .h 后缀
```

### 为什么 Windows 上"碰巧能编译"

```
NTFS（Windows 文件系统）：不区分大小写
  Vector == vector == VECTOR  → 都能找到文件

ext4/xfs（Linux 文件系统）：区分大小写
  Vector ≠ vector ≠ VECTOR  → 找不到 Vector
```

所以代码在 Windows 上能编译，push 到 Linux CI 就失败。

### C 头文件 vs C++ 包装

```cpp
// C++ 标准 C 头文件：c 前缀 + 全小写 + 无 .h
#include <cstdio>     // ✅ 不是 <cstdio.h> 或 <CSTDIO>
#include <cstdlib>
#include <cstring>
#include <cmath>

// C 原始头文件（C++ 中已废弃但可用）
#include <stdio.h>    // 能用但不推荐混用
#include <string.h>
```

### 第三方库头文件

第三方库的头文件大小写取决于库的设计，但标准库一定全小写：

```cpp
// 标准库：全小写
#include <iostream>

// 第三方：按库的文档来
#include <boost/asio.hpp>     // boost 用全小写
#include <Eigen/Dense>        // Eigen 用首字母大写
```

## 常见错误（新手踩坑）

### 错误 1：首字母大写

```cpp
#include <Vector>   // Windows OK, Linux 失败
```

**修复**：`#include <vector>`。

### 错误 2：加 .h 后缀

```cpp
#include <vector.h>  // 旧版编译器可能有，标准 C++ 没有
```

**修复**：`#include <vector>`（无 .h）。

### 错误 3：C 和 C++ 头文件混用

```cpp
#include <stdio.h>     // C 风格
#include <iostream>    // C++ 风格
#include <string.h>    // C 风格
#include <vector>      // C++ 风格
// 混用不一致，且 string.h 和 string 容易混淆
```

**修复**：统一用 C++ 风格 `<cstdio>`/`<cstring>`。

## 新手要点（和 C 的区别）

| 方面 | C | C++ |
|------|---|-----|
| 标准头文件 | `<stdio.h>`（有 .h） | `<cstdio>`（c 前缀，无 .h） |
| 大小写 | 通常小写 | **必须全小写** |
| .h 后缀 | 有 | 无（标准库） |
| 跨平台风险 | 存在 | 更高（头文件更多） |

## HFT 关联

- **CI/CD 跨平台编译**：HFT 系统通常在 Linux 上编译部署，头文件大小写错误在 Windows 开发时发现不了
- **统一编码规范**：项目规范要求标准头文件全小写，CI 脚本可加检查
- **避免 .h 后缀**：C++ 标准头不用 .h，避免与旧版编译器的不兼容

## 代码自测

### Q1: 大小写错误

```cpp
#include <Vector>
#include <String>
#include <Algorithm>

int main() {
    std::vector<int> v;
    std::string s;
    std::sort(v.begin(), v.end());
    return 0;
}
```
> 这段代码在 Windows 和 Linux 上分别能编译吗？

<details>
<summary>答案</summary>

- **Windows**：可能能编译（NTFS 不区分大小写，实际找到的是 `vector`/`string`/`algorithm`）
- **Linux**：**编译失败**（`fatal error: Vector: No such file or directory`）

**修复**：全改为小写。

```cpp
#include <vector>
#include <string>
#include <algorithm>
```
</details>

### Q2: C vs C++ 头文件

```cpp
// A:
#include <math.h>
// B:
#include <cmath>
// C:
#include <Math>
```
> 哪些写法是正确的？

<details>
<summary>答案</summary>

- **A `<math.h>`**：能用，C 风格头文件，函数在全局命名空间
- **B `<cmath>`**：✅ 推荐，C++ 风格，函数在 `std::` 命名空间
- **C `<Math>`**：❌ 错误，大写且非标准名

**规则**：C++ 代码统一用 `<cXXX>` 形式。
</details>

### Q3: .h 后缀

```cpp
#include <vector.h>
```
> 这行代码在标准 C++ 编译器上能编译吗？

<details>
<summary>答案</summary>

**不能**。标准 C++ 头文件没有 `.h` 后缀。

`<vector.h>` 是 C++ 标准化之前（C++98 之前）的旧式头文件，现代编译器不再提供。

**修复**：`#include <vector>`。

注意：C 头文件（如 `<stdio.h>`）在 C++ 中仍然可用（作为兼容），但 C++ 标准头（如 `<vector>`/`<string>`/`<algorithm>`）一律没有 .h。
</details>

### Q4: 完整修复

```cpp
// 这段代码有多少处头文件错误？
#include <Stdio.h>
#include <vector.h>
#include <String>
#include <Algorithm>

int main() {
    std::vector<std::string> v;
    std::sort(v.begin(), v.end());
    printf("done\n");
    return 0;
}
```

<details>
<summary>答案</summary>

4 处错误：
1. `<Stdio.h>` → `<cstdio>`（大写 + C 风格 → C++ 风格）
2. `<vector.h>` → `<vector>`（.h 后缀）
3. `<String>` → `<string>`（大写）
4. `<Algorithm>` → `<algorithm>`（大写）

修复后：
```cpp
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
```

**规则**：C++ 标准头文件——全小写、无 .h、C 头用 c 前缀。
</details>

## 参考与延伸

- 上一节：[Item 47 不依赖实现](item47-no-implementation-assumptions.md)
- 下一节：[Item 49 解读错误信息](item49-read-error-messages.md)
