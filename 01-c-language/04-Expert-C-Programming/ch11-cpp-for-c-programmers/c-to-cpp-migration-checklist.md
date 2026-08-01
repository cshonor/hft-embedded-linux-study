# C → 安全 C++ 对照改写清单

**Expert C ch11** — 从全书 C 底层习惯过渡到现代 C++ 的一页速查。

## 1. 预处理与常量

| C 写法 | C++ 推荐 | 原因 |
|--------|----------|------|
| `#define MAX 128` | `constexpr int MAX = 128;` | 类型安全、无宏副作用（ch02/ch08） |
| `#define MAX(a,b) ...` | `inline` 函数或 `std::max` | 无优先级/重复求值 UB |
| `#include <stdio.h>` | `#include <cstdio>` + `std::` | C++ 头命名规范 |

## 2. 内存管理

| C | C++ | 禁止 |
|---|-----|------|
| `malloc` / `free` | `new` / `delete` 或智能指针 | **混用**两套分配器 |
| `malloc(n*sizeof(T))` | `new T[n]` + `delete[]` | `free` 配 `new` |
| 裸 `char buf[N]` 字符串 | `std::string` | 无边界 `strcpy` |
| 手动扩容数组 | `std::vector<T>` | 泄漏 + 越界 |

## 3. 传参与返回值

| C | C++ |
|---|-----|
| `void f(int **p)` 改外部指针 | `void f(int *&p)` 或返回值 |
| `void f(int *p)` 大对象只读 | `void f(const T &p)` |
| 输出参数 `int *out` | 返回 `T` / `std::optional` / 引用出参 |

## 4. 类型与链接

| C | C++ |
|---|-----|
| `void *` 隐式赋给 `T*` | 必须 `(T*)` 或 `static_cast` |
| `const int x = 1;` 全局（C 外部链接） | C++ 默认 **内部链接**，需 `extern const` |
| C 函数供 C++ 调用 | 声明加 `extern "C"` |
| C++ 重载函数给 C 用 | 仅 `extern "C"` 包装 **无重载** 接口 |

## 5. 字符串与 IO

| C | C++ |
|---|-----|
| `char s[64]; fgets(s,...)` | `std::string line; std::getline(...)` |
| `printf("%d", x)` | `std::cout << x` 或 fmt 库 |
| `strcmp` / `strcpy` | `s1 == s2` / `s1 = s2`（string） |

## 6. 多态与扩展

| C（内核风格） | C++ |
|---------------|-----|
| `struct ops { void (*open)(...); };` | `class Base { virtual void open(); };` |
| 手动函数表 + `container_of` | `virtual` + vtable（编译器生成） |
| `struct` + 全局函数 | `class` 封装数据+方法 |

## 7. 底层/内核场景保留 C 风格

- 禁用或慎用：异常、RTTI、全局构造析构
- 热路径：无 `virtual`、无堆分配
- 驱动接口：`extern "C"` 导出
- 与 ch05 `kernel.elf`、ch10 函数指针表一致

## 8. 改写顺序（建议）

1. `#define` → `constexpr` / `inline`
2. `malloc/free` → `vector` / `string` / RAII
3. 二级指针出参 → 引用 / 返回值
4. 裸 `char*` → `std::string` / `string_view`
5. 需要多态时再引入 `virtual`，否则保留 C 函数表

## Demo

见 [demo/](./demo/README.md)。
