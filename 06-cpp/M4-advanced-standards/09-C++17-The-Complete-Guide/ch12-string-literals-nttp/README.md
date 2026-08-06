# 第 12 章 字符串字面量作模板参数

**Dealing with String Literals as Template Parameters**

## 本章讲什么

C++17 仍不能直接用字符串字面量做模板参数（C++20 才行），但本章讲几种变通方法实现"字符串编译期参数化"——用于策略名、日志标签等的编译期传递。

## 要点

### 问题：字符串字面量为什么不能直接做模板参数

```cpp
template <const char* N>
struct Named {};
Named<"foo"> x;   // 编译错！字符串字面量是 const char[N]，不是指针，且无链接
```

模板的非类型参数（NTTP）要求有**外部链接**的地址，字符串字面量是 `static` 链接的数组，不满足。

### 变通方法 1：外部链接的变量

```cpp
extern const char NAME[] = "foo";   // 外部链接
Named<NAME> x;   // OK
```

缺点：每个名字要在 namespace 作用域定义一个变量，啰嗦。

### 变通方法 2：固定长度字符数组

```cpp
template <size_t N>
struct FixedString {
    char data[N];
    constexpr FixedString(const char (&s)[N]) {
        std::copy_n(s, N, data);
    }
};

template <FixedString S>
struct Named {
    static constexpr const char* name = S.data;
};

Named<"foo"> x;   // C++17 OK！FixedString 是字面量类型，可做 NTTP
```

这是 C++17 最常用的方法。`FixedString` 是字面量类型（constexpr 构造），可作 NTTP。C++20 直接支持字符串字面量做 NTTP，此法就不必要了。

### 变通方法 3：`std::string_view`（运行期）

```cpp
constexpr std::string_view name = "foo";   // 但 string_view 不能做 NTTP（C++17）
```

`string_view` 在 C++17 不是字面量类型的 NTTP 候选（构造函数非 constexpr 直到 C++20 某些实现）。

### 用途

- **编译期策略名**：`Strategy<"Alpha">` 类型本身带名字，日志/调试无需存字符串。
- **编译期日志标签**：`Logger<"FEED">` 每个 logger 类型唯一，编译期解析。
- **编译期配置键**：`Config<"timeout">` 类型安全的键。

## HFT 关联

- **编译期策略名**：`Strategy<FixedString{"Alpha"}>` 让策略类型带名字，日志打印无需存运行期 string，零分配。
- **编译期日志标签**：`Logger<"FEED">` 不同模块不同 logger 类型，编译期区分，运行期无字符串比较。
- **FixedString 技巧是 C++17 的过渡**：C++20 直接支持字符串 NTTP，但 C++17 项目仍需此法。
- **零开销**：编译期处理，运行期只有固定字符串字面量，无堆分配。
- **类型安全**：`Strategy<"Alpha">` 和 `Strategy<"Beta">` 是不同类型，编译期防混淆。

## 自测题

1. 为什么 C++17 不能直接用字符串字面量做模板参数？
2. FixedString 变通方法的核心思路是什么？为什么它能做 NTTP？
3. 外部链接变量方法有什么缺点？
4. C++20 对字符串 NTTP 的支持有什么改进？
5. HFT 用编译期策略名有什么好处？运行期相比有什么优势？
