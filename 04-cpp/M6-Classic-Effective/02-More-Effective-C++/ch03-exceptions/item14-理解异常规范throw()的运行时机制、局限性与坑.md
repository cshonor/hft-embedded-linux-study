# 条款 14：理解异常规范的局限与坑（动态异常规范 → noexcept 的演变）

## 本节讲什么

**Understand exception specifications.** 原书（2005）讲的是**动态异常规范**
（`throw()`/`throw(TypeA, TypeB)`）——那套机制已被证明是失败设计：
C++11 废弃、C++17 **彻底移除**，由 `noexcept` 取代。
本条按 2026 视角重写：动态规范为什么失败、noexcept 的正确语义、
以及读老代码时 `throw()` 该翻译成什么。

← 上一条 [item13 catch(...) 兜底](./item13-合理使用catch(...)全局捕获，谨慎兜底.md)；
下一条 [item15 异常的成本](./item15-了解异常的成本开销，高性能场景下的取舍.md)。

---

## 1. 动态异常规范的失败史（为什么被移除）

```cpp
// C++98 的动态异常规范（已移除）：
void f() throw(std::bad_alloc);      // 承诺只抛 bad_alloc
void g() throw();                    // 承诺不抛——≠ noexcept！
void h();                            // 无规范：可以抛任何异常
```

**失败原因**：
- **运行期检查**：违反规范时调 `std::unexpected()`（默认 terminate）——
  检查成本在每次调用，收益却几乎为零
- **不防**"规范外的异常来自更深层"：f() 调用的函数抛了规范外异常，
  拦截点在 f 的边界——出错时现场已经很难看
- **泛型不兼容**：模板函数的异常集合依赖 T（`T` 的构造可能抛什么）——
  静态规范写不出来
- **阻碍优化**：编译器无法从 throw() 推出"不抛"（仍要生成 unexpected 拦截代码）

结论：静态类型系统管异常集合这条路走不通——
C++11 起换思路：**只回答"抛不抛"（noexcept），不回答"抛什么"**。

## 2. noexcept 的正确语义（18.1 ② 的延伸）

```cpp
void swap(Widget& a, Widget& b) noexcept;        // 承诺不抛——违约 = terminate
void f() noexcept(false);                        // 显式声明"可能抛"（默认语义）
template <typename T>
void g() noexcept(noexcept(T()));                // 条件 noexcept：随 T 的特性走
```

- **noexcept = "违约即 terminate"**——编译器据此**消掉展开代码**：
  noexcept 函数内 throw 直接 terminate，不需要准备 catch 现场——
  代码更小、更快（这才是 noexcept 的性能价值，→ 18.1 ② vector 扩容实测）
- **条件 noexcept** 是泛型世界的标准件：`noexcept(noexcept(...))` 双层——
  内层是"探测表达式"，外层是声明（→ item25 swap、19.8 traits 联动）

## 3. 老代码翻译表（读遗产代码必备）

| 老写法（C++98/03） | 现代等价 | 注意 |
|---|---|---|
| `void f() throw();` | `void f() noexcept;` | **语义近似但不同**：throw() 违约调 unexpected（可自定义），noexcept 直接 terminate |
| `void f() throw(A, B);` | **删除规范**（写文档注释） | 没有等价物——异常集合只能靠文档 |
| `virtual ~Base() throw();` | `virtual ~Base() noexcept = default;`（其实析构默认 noexcept） | 析构/移动/swqp 天然 noexcept |

## HFT 关联

- noexcept 是**性能注释**不只是正确性注释：vector 扩容的移动/拷贝选择
  （18.1 ② 实测）、移动构造的启用（→ M1）都由它驱动——
  你的值类型"移动 + swap + 析构"三件套必须 noexcept
- `-fno-exceptions` 的热路径库里 noexcept 是**默认状态**——
  但仍要显式标：库的边界（与异常世界的接口层）靠 noexcept 声明"不越界"
- 读老代码（muduo/老 SDK）遇到 `throw()` 直接脑内翻译 noexcept——
  遇到 `throw(SomeException)` 知道它没有现代等价物，
  信息只能从文档/实现里挖（→ item54 的"熟悉标准库"同款：老接口的考古能力）

## 代码自测

**题目 1：** 动态异常规范（`throw(A, B)`）为什么失败？noexcept 用什么思路替代它？

<details>
<summary>参考答案</summary>

动态规范试图用**静态类型系统**管理"抛什么"——但运行期检查（unexpected）
成本高、泛型不兼容（T 决定异常集合）、违反时的现场已经狼藉。
noexcept 换思路：**只答"抛不抛"，不答"抛什么"**——
异常的具体类型本来就是 catch 端的事（按类型分派，item12），
函数签名只需承诺"我会不会成为异常源"。
这个简化让编译器能真正利用它（消展开代码、移动语义启用），
而"抛什么"回归文档和 catch 端处理。
（Java 的 checked exceptions 是同一思路的另一个失败案例——
异常集合不属于类型系统。）

</details>

**题目 2：** `noexcept` 函数里抛了异常会怎样？这和 `throw()` 的老行为差在哪？

<details>
<summary>参考答案</summary>

`noexcept` 函数抛出异常 → **`std::terminate` 立即调用**——
无栈展开、无 catch 机会、默认 abort（可经 set_terminate 自定义终结动作，
但也只能"死前做点事"，救不回来）。
老 `throw()` 违约调 `std::unexpected()`——默认也是 terminate，
但允许 set_unexpected 换处理器（理论上能转换异常类型重抛，实践中没人用）。
语义近似，细节不同；真正重要的区别是**编译器的利用**：
noexcept 让编译器**消掉异常处理代码**（更小更快），
throw() 反而要生成 unexpected 拦截代码（更大更慢）——
这就是新机制取代旧机制的根本原因。

</details>

**题目 3：** `template <typename T> void wrapper(T& a, T& b) noexcept(noexcept(a.swap(b)));`
里两个 noexcept 各是什么？

<details>
<summary>参考答案</summary>

外层 `noexcept(...)`：**声明**——wrapper 的 noexcept 属性由括号里的
布尔表达式决定（编译期求值）。
内层 `noexcept(expr)`：**探测运算符**——编译期回答"表达式 expr 承诺不抛吗"
（检查 a.swap(b) 这个调用的 noexcept 声明，不真的执行）。
合起来的语义："wrapper 的不抛承诺 = 成员 swap 的不抛承诺"——
T 的 swap 是 noexcept 则 wrapper 也是，T 的会抛则 wrapper 不承诺。
这就是**条件 noexcept**：泛型代码的异常承诺随类型参数的特性走——
swap/移动/容器的标准库实现里无处不在（→ item25、19.8 traits 的合体应用）。

</details>
