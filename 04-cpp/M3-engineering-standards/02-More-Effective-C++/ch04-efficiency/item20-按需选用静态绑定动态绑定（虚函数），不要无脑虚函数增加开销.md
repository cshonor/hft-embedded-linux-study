# 条款 20：按需选用静态绑定/动态绑定（虚函数），不要无脑虚函数增加开销

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Base { public: virtual void f(); };
class Derived : public Base {};
void use(Base &b) { b.f(); }  // 需要多态才用 virtual
```

---

## 代码自测

**题目 1：** 虚函数有哪些性能开销？
```cpp
class Base { public: virtual void f() {} };
class Derived : public Base { public: void f() override {} };
```

<details>
<summary>参考答案</summary>

虚函数开销：1) 每个 vtable 指针（每对象 +8 字节）；2) 虚调用需两次间接（读 vtable 指针 + 读函数指针），可能影响分支预测和指令缓存；3) 阻止内联（编译器通常不对虚调用做内联，因为实际调用目标在运行期才确定）。非虚函数直接调用、可内联。HFT 热路径应避免虚函数，用模板/CRTP 做编译期多态。

</details>
