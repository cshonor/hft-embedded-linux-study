# Item 5：优先用 auto 而非显式类型声明

> 第 2 章 auto · Item 5 · 下一节：[Item 6 显式类型初始化习惯](item06-explicitly-typed-initializer.md)

## 这节讲什么

`auto` 不只是"少打几个字母"的语法糖。它在**正确性**（避免隐式转换切片）、**可维护性**（重构时类型自动跟随）、**性能**（避免意外拷贝）三个维度都有真实价值。

---

## 四大优势

1. **避免未初始化变量**：`auto x;` 编译失败（推不出类型），强制你写 `auto x = expr;`——天然防忘初始化。
   ```cpp
   int x;           // UB：未初始化，值不确定
   auto x;          // 编译失败！
   auto x = 42;     // OK
   ```

2. **避免类型不匹配的隐式转换**：
   ```cpp
   unsigned sz = vec.size();   // 隐式窄化：size_t→unsigned，大容器截断
   auto sz = vec.size();       // size_t，零风险
   ```

3. **重构友好**：函数返回类型变了，`auto` 调用方零改动；显式类型要逐处改。

4. **闭包/lambda 类型无法手写**：
   ```cpp
   auto f = [x](int n){ return x*n; };  // lambda 类型由编译器合成，没有名字
   ```

---

## 新手要点（和 C 的区别）

- **C 必须手写类型**，C 程序员习惯 `int x = 42;`。C++ 里 `auto x = 42;` 等价但更安全（防隐式窄化）。
- **什么时候别用 auto**：类型简单且想明确表达意图时（`int count = 0;` 比 `auto count = 0;` 更直白）。读代码时 `auto` 隐藏了类型，降低可读性。
- **`auto` 不等于 `var`**（不是动态类型）——它编译期就确定了，只是"让编译器帮你写类型"。

---

## HFT 关联

- **避免订单数量截断**：`auto q = order.qty();` 比 `int q = order.qty();` 安全——后者在 `qty()` 返回 `int64_t` 时静默截断，导致下单数量错误（资损）。
- **lambda 回调注册**：`auto cb = [this](const Tick& t){ ... };` 是 HFT 策略引擎注册回调的标准写法。

---

## 自测题

1. `auto x;` 为什么编译不过？这如何帮助避免未初始化变量？
2. `unsigned sz = vec.size()` 和 `auto sz = vec.size()` 在 `vec` 元素数超过 `UINT_MAX` 时行为有何不同？
3. 为什么 lambda 的类型无法手写，只能用 `auto`？
4. 举一个"重构时 auto 比显式类型更省事"的例子。

---

## 参考与延伸

- 下一节：[Item 6 显式类型初始化习惯](item06-explicitly-typed-initializer.md)
- 回到：[第 2 章 auto](README.md)
