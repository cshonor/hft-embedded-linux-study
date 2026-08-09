# 条款 24：理解虚函数、多重继承带来的内存布局、开销、歧义问题

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class A { virtual void f(); };
class B : public A { void f() override; };
// 多重继承时注意虚表与对象布局
```

---

## 代码自测

**题目 1：** 多重继承 + 虚函数的内存布局有什么开销？
```cpp
class A { public: virtual void fa() {} int a; };
class B { public: virtual void fb() {} int b; };
class C : public A, public B { public: void fa() override {} void fb() override {} int c; };
sizeof(C)  // 大约多少？
```

<details>
<summary>参考答案</summary>

约 24 字节（64 位）：vptr_A + int a + padding + vptr_B + int b + int c + padding ≈ 8+4+4+8+4+4 = 32 字节（含对齐）。C 有两个 vtable 指针（分别对应 A 和 B 的子对象）。多重继承的内存布局更复杂，`C*` 转 `B*` 时指针需要偏移调整。虚继承更重——需要虚基类指针/虚基类表。HFT 场景应避免多重继承和虚继承。

</details>
