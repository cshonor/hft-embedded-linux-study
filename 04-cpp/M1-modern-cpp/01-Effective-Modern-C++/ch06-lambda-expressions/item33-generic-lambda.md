# Item 33：对 auto&& 形参用 decltype + std::forward（泛型 lambda）

> 第 6 章 · Item 33 · 上一节：[Item 32 初始化捕获](item32-init-capture.md)

## 为什么要学这个（先建立直觉）

函数模板的完美转发你已经会了：`template<class T> void f(T&& x) { g(std::forward<T>(x)); }`。C++14 把泛型能力给了 lambda（参数可以写 `auto`），问题随之而来：**lambda 没有你能写的模板参数名 `T`**——`forward` 需要显式模板实参，`forward<???>(x)` 里填什么？

Meyers 的答案：`decltype(x)`。形参 `x` 的声明类型里带着推导出的引用性，`decltype` 原样取出（Item 3 的规则在这里派上用场），交给 `forward` 正好够用。这是第 1 章（decltype）、第 5 章（万能引用 + forward）两条线在 lambda 里的合流点。

---

## 这节讲什么

1. 泛型 lambda 完美转发的标准写法与实测（左值/右值各走各的重载）；
2. 为什么必须用 `decltype(x)`、为什么 `std::move` 是错的（实测对照）；
3. C++20 的显式模板参数 lambda（终于能写出 `T` 了）。

---

## 核心用法与实测

```cpp
auto fwd = [](auto&& x){
    func(std::forward<decltype(x)>(x));
};
```

`auto&&` 是万能引用（有 `auto` 推导，[Item 24](../ch05-rvalue-move-forwarding/item24-universal-vs-rvalue.md)）：左值实参 → `auto` 推为 `T&`（形参折叠成 `T&`）；右值实参 → `auto` 推为 `T`（形参为 `T&&`）。`decltype(x)` 取形参的**声明类型**——左值实参时是 `T&`，右值实参时是 `T&&`——正是 `forward` 需要的输入。

clang 23.1.0 实测（重载分辨值类别）：

```cpp
void func(std::string& s)  { std::cout << "左值重载: " << s << "\n"; }
void func(std::string&& s) { std::cout << "右值重载: " << s << "\n"; }

auto fwd = [](auto&& x){ func(std::forward<decltype(x)>(x)); };
std::string s = "tick";
fwd(s);                    // 左值重载: tick   ← 左值保持左值
fwd(std::string("新生"));   // 右值重载: 新生   ← 右值保持右值
```

两个重载各走各的，转发是完美的。

## 反面教材：用 std::move 代替 forward

```cpp
auto bad = [](auto&& x){ func(std::move(x)); };
bad(s);                    // 右值重载: tick   ← 左值被强制转成右值！
```

`std::move` 是**无条件**转右值——左值实参也被强行移动，调用方的对象在不知情中被"允许掏空"。实测里 `bad(s)` 走了右值重载，证明 `s` 被当右值处理了。

附带一个诚实的实测细节：打印 `bad` 之后的 `s`，本机（libc++ SSO）仍显示 `"tick"`——move 后源对象是**有效但未指定**状态，libc++ 的短字符串优化恰好没清它。**"看起来没被掏空"不是安全的证据**——换长字符串（堆分配）或别的实现就清空了。正确性不能赌实现细节。

## 为什么不能写 forward&lt;T&gt;

lambda 的闭包类型里 `operator()` 确实是模板，但那个模板参数**没有你能写的名字**：

```cpp
// 闭包类型大致长这样（编译器合成）：
class Closure {
public:
    template<typename __T>
    auto operator()(__T&& x) const { ... }
};
```

`__T` 是编译器的内部名，你的 lambda 体里够不着它。`decltype(x)` 是唯一能合法拿到这个类型的途径。

## C++20：终于能写 T 了

C++20 允许 lambda 带显式模板参数列表（clang 23.1.0 + `-std=c++20` 实测）：

```cpp
auto f = []<typename T>(T&& x){
    func(std::forward<T>(x));   // T 有名字了，forward<T> 直写
};
```

实测推导与模板一致：左值实参 `[T = int &]`、右值实参 `[T = int]`。C++20 项目里这是更直白的写法；C++14/17 代码库里 `decltype(x)` 写法仍是标准答案。

---

## 常见错误（新手踩坑）

1. **泛型 lambda 里用 `std::move`**。如实测：左值也被转右值，调用方对象被掏空（而且 SSO 下可能"看起来没事"，bug 会潜伏到换数据/换平台才爆）。
2. **试图 `forward<auto>(x)`**——`auto` 不是类型，不能当模板实参。`decltype(x)` 是唯一通道（C++20 前）。
3. **`decltype((x))` 加括号**。Item 3 的括号陷阱在这里同样存在：`decltype((x))` 对左值形参给 `T&`……和 `decltype(x)` 恰好相同？不——`x` 是**变量名**，`decltype(x)` 给声明类型（含推导出的引用性）；`(x)` 是表达式，左值表达式恒给 `T&`——右值实参的 `x`（声明类型 `T&&`）经 `(x)` 只剩 `T&`，右值性丢了。转发里永远写 `decltype(x)`，不多不少。
4. **普通 lambda 过度泛化**。参数类型明明确定（只接 `const Tick&`），就别写 `auto&&`——泛型接口的接受面越大，误用面越大。

---

## 新手要点

- **版本线**：C++11 lambda 参数必须显式类型；C++14 起支持 `auto`（泛型 lambda）；C++20 起支持显式模板参数列表 `[]<class T>(T&& x)`。
- **记忆钩子**：lambda 里没有 `T`，有 `x`——`forward<decltype(x)>(x)`，`decltype` 就是你的 `T`。
- **什么时候用**：写"把实参原样递给下一层"的转发 lambda（回调适配、异步包装）。普通 lambda 不需要这套。

---

## HFT 关联

- **回调适配层**：策略引擎里把用户 lambda 包一层再接进事件循环（记日志、计时、异常吞掉）——包装层必须完美转发，否则 tick 对象在每一跳被多拷一次。`[](auto&& tick){ timer.scoped([&]{ user_cb(std::forward<decltype(tick)>(tick)); }); }` 是标准件。
- **decltype 全家桶的汇合点**：Item 3 的 `decltype` 规则 + Item 24 的万能引用 + Item 25 的 forward，在泛型 lambda 这一个语法点上全部用上——它是检验前三章是否真懂的试金石。

---

## 自测题

1. 泛型 lambda 的 `auto&&` 参数如何配合 `std::forward` 实现完美转发？为什么不能写 `forward<T>`？
2. `decltype(x)` 在 `auto&&` 形参上拿到什么？左值实参和右值实参时分别是什么？
3. 用 `std::move` 代替 `forward<decltype(x)>` 会发生什么？实测里有什么容易误导读者的细节？
4. `decltype(x)` 和 `decltype((x))` 在转发语境下有何区别？哪个对？
5. C++20 给泛型 lambda 加了什么新写法？

<details>
<summary>参考答案</summary>

1. `auto&&` 是万能引用：左值实参使 `auto` 推为 `T&`（形参折叠成 `T&`），右值实参推为 `T`（形参 `T&&`）。`decltype(x)` 取出形参的声明类型——左值时是 `T&`、右值时是 `T&&`——交给 `std::forward<decltype(x)>(x)` 即可还原值类别（实测：左值走左值重载、右值走右值重载）。不能写 `forward<T>` 是因为 lambda 的模板参数是编译器合成的内部名，源码里没有可写的 `T`。

2. 拿到形参 `x` 的声明类型，完整保留引用性与 cv：左值实参时是 `T&`，右值实参时是 `T&&`。它不像 `auto` 那样剥引用——这正是 Item 3 的 `decltype` 分支一规则（变量名 → 声明类型）在起作用。

3. 左值实参也被强制转右值：`bad(s)`（`s` 是左值）实测走了右值重载，`s` 被"允许掏空"。误导细节：移动后打印 `s` 在本机仍显示 `"tick"`——libc++ 的 SSO 短字符串移动后保留了内容，但标准只保证"有效未指定"；换长字符串（堆分配）或换实现就清空。"没看到破坏"不等于"没有破坏"。

4. `decltype(x)` 取变量名的声明类型（`T&` 或 `T&&`，含推导出的引用性）——正确；`decltype((x))` 把 `(x)` 当表达式，左值表达式恒给 `T&`，右值实参的 `T&&` 被压成 `T&`，右值性丢失、forward 退化成左值转发。转发语境永远写 `decltype(x)`，不多不少。

5. 显式模板参数列表：`[]<typename T>(T&& x){ func(std::forward<T>(x)); }`——模板参数终于有了可写的名字，`forward<T>` 直写，与函数模板写法完全对齐（clang 23.1.0 + `-std=c++20` 实测通过，推导结果与 `auto&&` 版一致）。

</details>

---

## 参考与延伸

- 下一节：[Item 34 lambda vs bind](item34-lambda-vs-bind.md)
- 回到：[第 6 章](README.md)
