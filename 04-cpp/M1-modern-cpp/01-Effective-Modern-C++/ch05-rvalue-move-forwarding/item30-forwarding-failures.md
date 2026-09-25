# Item 30：熟悉完美转发失败的处境

> 第 5 章 · Item 30 · 上一节：[Item 29 移动不存在/廉价](item29-move-not-exist.md)

## 这节讲什么

完美转发在以下场景失败——知道这些边界条件才能在泛型代码里正确排查问题。

---

## 转发失败的场景

1. **大括号初始化**：`{1,2,3}` 无法转发（模板不推导 braced-init-list）
   ```cpp
   template<class T> void fwd(T&& x) { target(std::forward<T>(x)); }
   fwd({1, 2, 3});  // 编译失败！
   // 变通：auto il = {1,2,3}; fwd(il);
   ```

2. **0 或 NULL 当空指针**：推导为 `int` 而非指针
   ```cpp
   fwd(0);     // T 推为 int，不是 nullptr
   fwd(nullptr);  // OK
   ```

3. **重载的函数指针**：无法确定转发哪个重载
   ```cpp
   void f(int);
   void f(double);
   fwd(f);  // 编译失败！哪个 f？
   // 变通：fwd(static_cast<void(*)(int)>(f));
   ```

4. **位字段**：无法绑定非 const 引用到位字段
   ```cpp
   struct Bits { unsigned b : 1; };
   Bits bits;
   fwd(bits.b);  // 编译失败！
   ```

---

## 新手要点

- **完美转发不是万能的**：碰到 braced-init-list、0/NULL、重载函数指针、位字段时会失败。
- **变通方法**：每种失败都有对应的绕过方式（显式转 `initializer_list`、用 `nullptr`、`static_cast` 指定重载、拷贝位字段到临时变量）。

---

## HFT 关联

- **泛型包装器**：写 `template<class... Args> void log(Args&&... args)` 时，传 `{1,2,3}` 会失败——要提前知道这些边界。

---

## 自测题

1. 完美转发失败有哪四种场景？
2. `fwd({1, 2, 3})` 为什么编译失败？如何变通？
3. `fwd(0)` 推导出什么类型？应该用什么替代？
4. 重载函数指针为什么无法完美转发？

<details>
<summary>参考答案</summary>

1. 完美转发（forwarding）失败的四种典型场景：①**braced-init-list**（`{1,2,3}` 这类花括号初始化列表，模板无法推导）；②**`0` 或 `NULL` 当空指针**（被推成整型而非指针类型）；③**重载函数名 / 函数模板名**（没有确定的类型，无法推导）；④**位字段**（bit-field，无法绑定非 const 引用）。此外还有重载名相关的模板名、以及某些情况下的静态常量成员等边角情况，但上面四类是最常遇到的。

2. 因为 braced-init-list 不是表达式、没有类型，而模板推导只能作用于"有类型的实参"。`fwd(T&& x)` 的 `T` 无法从 `{1,2,3}` 推导出来，编译在推导阶段就失败（这与 Item 2 中"模板不能推导 braced-init-list 而 `auto` 可以"是同一条规则）。变通方法：先在调用侧用一个具名变量接住它，再转发该变量——
```cpp
auto il = {1, 2, 3};      // std::initializer_list<int>
fwd(il);                  // OK：有确定类型
```
或者直接显式指定目标类型：`fwd(std::initializer_list<int>{1,2,3});`。

3. `fwd(0)` 推导出 `T = int`（`0` 是整型字面量），形参类型是 `int&&`，转发给目标函数时是一个 `int` 右值，而不是空指针——如果目标期望指针就会编译失败或语义错误。应该用 `nullptr`：它的类型是 `std::nullptr_t`，能正确转换成任意指针类型，表达"空指针"意图。用 `NULL` 同样不行，因为 C++ 里 `NULL` 通常就是 `0`/`0L`，是整型。

4. 因为重载函数名本身不是一个"值"，而是**一组候选函数的名字**：它没有唯一的类型，`fwd(f)` 时编译器不知道该把 `f` 解析成 `void(*)(int)` 还是 `void(*)(double)`，模板参数 `T` 无从推导（推导需要先确定实参的类型）。同理，未指定实参的函数模板名也没有确定类型。变通：在调用侧显式指定重载——
```cpp
fwd(static_cast<void(*)(int)>(f));   // 明确选 void f(int)
```
对函数模板则写成 `fwd(static_cast<int(*)(int)>(&tmpl<int>));` 之类，先把名字解析成具体函数指针再转发。

</details>

---

## 参考与延伸

- 下一章：[第 6 章 Lambda 表达式](../ch06-lambda-expressions/README.md)
- 回到：[第 5 章](README.md)
