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

---

## 参考与延伸

- 下一章：[第 6 章 Lambda 表达式](../ch06-lambda-expressions/README.md)
- 回到：[第 5 章](README.md)
