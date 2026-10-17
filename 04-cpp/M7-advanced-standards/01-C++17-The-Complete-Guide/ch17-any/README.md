# 第 17 章 std::any

**std::any**

## 本章讲什么

`std::any` 是类型擦除的容器——能持有**任意类型**的值，运行期通过 `type_info` 知道存的是什么。当类型集无法在编译期穷举时用 any，否则优先用 variant。

## 要点

### 基本用法

```cpp
#include <any>

std::any a = 42;           // 存 int
a = "hello";               // 换成 const char*
a = std::vector<int>{1,2}; // 换成 vector<int>

// 访问
if (a.type() == typeid(int)) {
    int i = std::any_cast<int>(a);   // 类型错抛 bad_any_cast
}

// 非抛访问
int* p = std::any_cast<int>(&a);     // 类型错返回 nullptr
```

### 与 variant 的区别

| 维度 | `variant<T1,...,Tn>` | `any` |
|------|----------------------|-------|
| 类型集 | 编译期固定 | 任意 |
| 类型安全 | 编译期 | 运行期（type_info） |
| 存储 | 内联（无堆分配） | 小对象优化，大对象堆分配 |
| 访问 | `visit`/`get` | `any_cast`（运行期检查） |
| 适用 | 类型已知 | 类型未知（如配置、脚本桥） |

### 存储与 SBO

`std::any` 内部用**小对象优化（SBO）**：
- 小对象（通常 ≤ 2-3 个指针大小）：内联存储，无堆分配。
- 大对象：堆分配。

```cpp
std::any a = 42;              // 内联（int 小）
std::any b = std::string(1000, 'x');  // 堆分配（string 大）
```

### `any_cast` 的语义

```cpp
std::any a = 42;

int i = std::any_cast<int>(a);           // 拷贝出 int，类型错抛异常
const int* p = std::any_cast<int>(&a);   // 返回内部 int 的指针，类型错返回 nullptr
```

`any_cast<T>(any*)` 返回内部指针（零拷贝），`any_cast<T>(any&)` 返回拷贝。

### 用途

- **配置系统**：`map<string, any>` 存任意类型配置值（int、string、vector 等）。
- **脚本/反射桥**：把 C++ 对象传给脚本层，类型运行期解析。
- **事件系统**：事件携带任意 payload。

## HFT 关联

- **配置系统用 any**：`unordered_map<string, any> config` 存不同类型参数，`any_cast<int>(config["timeout"])` 取值。
- **热路径慎用 any**：`any_cast` 有运行期 type_info 比较 + 可能堆分配，热路径用 variant（编译期类型集）。
- **SBO 利好小类型**：`any` 存 int/double 小对象内联，无堆分配，但仍比 variant 多 type_info 开销。
- **variant 优先**：HFT 消息体类型集编译期已知（Tick/Trade/OrderBook），用 variant 不用 any。
- **any 用于管理面**：策略参数热加载、监控元数据这类非热路径用 any 灵活，热路径用 variant 高效。

## 自测题

1. `any` 和 `variant` 的核心区别是什么？什么时候用哪个？
2. `any_cast<int>(a)` 和 `any_cast<int>(&a)` 的区别？
3. `any` 的 SBO（小对象优化）是什么？大小阈值是多少？
4. 为什么 HFT 热路径用 variant 不用 any？
5. 配置系统为什么适合用 `map<string, any>`？

## 代码自测

### Q1: any vs variant
```cpp
// any: 任意类型，运行时类型擦除
std::any a = 42;
a = "hello";  // 可以存任意类型
int* p = std::any_cast<int>(&a);  // nullptr（当前不是 int）

// variant: 固定类型集，编译期已知
std::variant<int, std::string> v = 42;
// v = 3.14;  // 编译错误：double 不在类型集中
```
> any 和 variant 的核心区别是什么？何时选哪个？

<details>
<summary>答案与复习指引</summary>

| 特性 | `any` | `variant<T1,T2,...>` |
|------|-------|---------------------|
| 类型集 | 任意（运行时） | 固定（编译期） |
| 类型安全 | 弱（any_cast 运行时检查） | 强（编译期检查） |
| 性能 | 堆分配（小对象可能 SSO） | 无堆分配（栈上联合） |
| 访问 | `any_cast<T>` | `get<T>`/`visit` |
| 大小 | 固定 sizeof（通常 16-32B） | max(sizeof(Ts)) + index |

**选择**：
- 类型在编译期已知 → `variant`（安全、高效）
- 类型在运行时才知道（如解析 JSON/配置）→ `any`
- HFT：热路径用 variant，避免 any 的堆分配和运行时类型检查

**复习：** → [any](./README.md)
</details>
