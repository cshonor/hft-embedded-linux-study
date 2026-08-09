# 第 7 章 新属性与属性扩展

**New Attributes and Attribute Features**

## 本章讲什么

C++17 新增三个属性：`[[nodiscard]]`、`[[maybe_unused]]`、`[[fallthrough]]`，并让属性能用在更多位置（如 namespace、enum、基类）。

## 要点

### `[[nodiscard]]`：返回值不能忽略

```cpp
[[nodiscard]] int compute() { return 42; }
compute();   // 警告：返回值被丢弃

[[nodiscard]] class Result {};   // 类标记 nodiscard，所有返回该类的函数都生效
Result r = make();   // OK
make();              // 警告

[[nodiscard("reason")]] int foo();   // C++20 起可带原因
```

用途：错误码、资源句柄、重要计算结果——忽略返回值通常是 bug。

### `[[maybe_unused]]`：消除"未使用"警告

```cpp
void foo([[maybe_unused]] int debug_level) {
    #ifdef DEBUG
    log(debug_level);
    #endif
}

[[maybe_unused]] static int counter = 0;   // release 模式可能不用
```

替代了 `(void)var;` 和 `__attribute__((unused))`，标准化、可移植。

### `[[fallthrough]]`：switch 显式贯穿

```cpp
switch (x) {
    case 1:
        prep();
        [[fallthrough]];   // 告诉编译器：这里故意贯穿，不要警告
    case 2:
        run();
        break;
}
```

C++17 之前，switch 不写 break 编译器会警告"可能漏写 break"。`[[fallthrough]]` 明确表示"故意贯穿"。

### 属性位置的扩展

C++17 允许属性写在更多地方：

```cpp
// namespace 级
namespace [[deprecated]] OldAPI { ... }

// enum / enum 值
enum class Color {
    Red,
    Green [[deprecated("use Teal")]],
    Blue
};

// 基类
struct Derived : [[deprecated]] Base { ... };
```

## HFT 关联

- **`[[nodiscard]]` 守护错误码**：风控检查函数 `[[nodiscard]] bool check()` 强制调用方处理结果，避免漏检。
- **`[[nodiscard]]` 守护资源句柄**：`[[nodiscard]] FD open()` 防止忘存文件描述符导致泄漏。
- **`[[maybe_unused]]` 管理调试变量**：`debug_level`、`trace_id` 这类 release 模式不用的变量标记后无警告。
- **`[[fallthrough]]` 状态机**：策略状态转换 switch 中故意贯穿的场景（如"初始化→就绪"共享部分逻辑）用 fallthrough 标注。
- **`[[deprecated]]` 标记旧接口**：策略 API 升级时标记旧函数，编译期警告提醒迁移。

## 自测题

1. `[[nodiscard]]` 的作用是什么？用在类上和用在函数上有什么区别？
2. `[[maybe_unused]]` 替代了哪些旧写法？
3. `[[fallthrough]]` 为什么不是"不写 break"的同义词？它的正确位置在哪？
4. C++17 允许属性写在哪些新位置？
5. HFT 风控函数为什么加 `[[nodiscard]]`？

## 代码自测

### Q1: 常用属性
```cpp
[[nodiscard]] int compute() { return 42; }
[[maybe_unused]] int debug_var = 0;
[[fallthrough]] switch(int x) {
    case 1: step1();
    [[fallthrough]];
    case 2: step2(); break;
}

auto r = compute();  // A: OK
compute();           // B: 警告
```
> B 行为什么会有警告？三个属性分别解决什么问题？

<details>
<summary>答案与复习指引</summary>

| 属性 | 作用 | 解决的问题 |
|------|------|-----------|
| `[[nodiscard]]` | 返回值不能忽略 | 忘记检查返回值（如错误码、分配结果） |
| `[[maybe_unused]]` | 抑制"未使用"警告 | 条件编译中有时用有时不用的变量 |
| `[[fallthrough]]` | 标记 switch 有意穿透 | 告诉编译器"我知道在穿透，不要警告" |

**B 行警告**：`compute()` 标记了 `[[nodiscard]]`，丢弃返回值 → 编译器警告"返回值被丢弃"。

**复习：** → [标准属性](./README.md)
</details>
