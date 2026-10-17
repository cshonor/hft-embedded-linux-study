# 核心语言改进

## 指定初始化

```cpp
struct Point { int x; int y; int z = 0; };

// C++20：指定初始化（按声明顺序）
Point p = {.x = 1, .y = 2};
Point p2 = {.x = 1, .y = 2, .z = 3};

// 不能跳过或重排
// Point p3 = {.y = 2, .x = 1};  // ❌ 顺序不对
// Point p4 = {.x = 1, .z = 3};  // ❌ 跳过 y

// 优势：明确每个值的含义
struct Config {
    int max_orders;
    double risk_limit;
    bool enable_logging;
};
Config cfg = {.max_orders = 100, .risk_limit = 0.05, .enable_logging = true};
// 比 Config{100, 0.05, true} 清晰
```

## consteval 和 constinit

```cpp
// 详见第 18 章
consteval int compile_only(int n) { return n * 2; }
constinit int x = compile_only(21);
```

## 运算符优先级调整

```cpp
// C++20： spaceship 优先级在 < 和 == 之间
// a <=> b < 0  →  (a <=> b) < 0  // 正确
```

## 更宽松的 constexpr

```cpp
// 详见第 18 章
// constexpr 可用循环、try/catch、std::vector 等
```

## using enum

```cpp
enum class Color { Red, Green, Blue };

// C++20：using enum 引入所有枚举值
void foo() {
    using enum Color;
    auto c = Red;   // 不用写 Color::Red
    auto c2 = Blue;
}
```

## 字符集改进

```cpp
// C++20：char8_t 类型
char8_t c = u8'A';  // UTF-8 字符
const char8_t* s = u8"hello";  // UTF-8 字符串

// C++17：u8 返回 const char*
// C++20：u8 返回 const char8_t*
```

## 自测题

1. 指定初始化 `{.x = 1, .y = 2}` 的规则是什么？能跳过成员吗？
2. `using enum` 做什么？
3. `char8_t` 是 C++20 新增的类型吗？为什么需要？
4. C++20 的 spaceship 运算符优先级在哪里？
5. 指定初始化相比聚合初始化 `{1, 2}` 有什么优势？

<details>
<summary>参考答案</summary>

1. 规则如下：
   1. 只能用于**聚合体**的聚合初始化，designator 写成 `.成员名 = 值`，并且必须引用**直接的**非静态数据成员（不能嵌套指定基类或子对象的成员）。
   2. 各 designator 必须按成员在类中的**声明顺序**出现，**不能乱序**。
   3. **可以跳过**成员——被跳过的成员会被**值初始化**（类类型调默认构造，标量置 0），而不是留下未初始化的值。
   4. 不能与位置初始化**混用**（`{.x = 1, 2}` 在 C++20 中不允许），同一成员也不能重复指定。
```cpp
Config cfg = {.max_orders = 100, .risk_limit = 0.05, .enable_logging = true};
```
2. 它是 C++20 新增的 using 形式，把某个枚举类型的**全部枚举项名字**引入当前作用域，之后可以直接写枚举项名字而不必加限定：
```cpp
enum class Color { Red, Green, Blue };
void foo() {
    using enum Color;
    auto c  = Red;    // 不用写 Color::Red
    auto c2 = Blue;
}
```
它只在块/类/命名空间作用域内生效，到作用域结束为止；比 `using namespace` 更精确（只放一个枚举的名字），也让 `switch` 分支和常量书写不再冗长。若只想引入个别枚举项，用 `using Color::Red;` 而不是 `using enum`。
3. 是。C++20 新增了独立的基础类型 **`char8_t`**：它的大小、符号性、对齐与 `unsigned char` 相同，但**是不同的类型**（重载决议会区分它）。
为什么需要：给 UTF-8 一个**专属类型**，把"UTF-8 文本"和"任意字节/本地编码文本"在类型系统层面区分开。
配套的变化是 `u8"..."` 字面量的类型从 C++17 的 `const char[]` 变成了 `const char8_t[]`（并新增 `std::u8string`），于是重载、`auto` 推导、编码转换库都能正确识别 UTF-8，不会再有人误把 UTF-8 字节串当本地编码处理。
4. `<=>` 与 `<`、`>`、`<=`、`>=` **同一优先级**（关系运算符层级），**高于** `==` / `!=`，并且**左结合**。
因此 `a <=> b < 0` 解析为 `(a <=> b) < 0`——先做三路比较，再把比较类别与 0 比较，这正是常见的判断写法。
（注：笔记里"优先级在 `<` 和 `==` 之间"容易被理解成"低于 `<`"；准确说法是与 `<` **同级**、高于 `==`，所以上面那个表达式才成立。）
5. 优势：
   1. **自文档化**：`.max_orders = 100` 直接写明哪个字段是什么值，`Config{100, 0.05, true}` 要靠读者去数位置；
   2. **不必依赖成员顺序记忆**：写错顺序也不会把值塞进错误的字段；
   3. **可以只初始化关心的成员**，其余自动值初始化，不必为了给最后一个字段赋值而把中间字段全写一遍；
   4. **维护性更好**：结构体增删/调整成员顺序时，位置初始化会**静默地**把值错位（最危险的 bug），而指定初始化要么仍然正确、要么直接编译报错。

</details>
