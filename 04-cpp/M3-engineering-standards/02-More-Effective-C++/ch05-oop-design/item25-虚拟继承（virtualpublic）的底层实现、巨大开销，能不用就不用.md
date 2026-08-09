# 条款 25：虚拟继承（virtual public）的底层实现、巨大开销，能不用就不用

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Base {};
class A : virtual public Base {};
class B : virtual public Base {};
class C : public A, public B {};  // 虚拟继承解决菱形，但有开销
```

---

## 代码自测

**题目 1：** 虚拟继承的底层实现为什么开销大？
```cpp
class A { public: int a; };
class B : virtual public A {};
class C : virtual public A {};
class D : public B, public C {};
```

<details>
<summary>参考答案</summary>

虚拟继承保证 D 中只有一份 A 子对象。实现方式：B 和 C 中各增加一个虚基类指针（或虚基类表条目），指向共享的 A 子对象。访问 `a` 时需要通过指针间接寻址——比普通成员访问多一次间接。同时构造/析构顺序更复杂。虚拟继承的额外开销：每对象多 8-16 字节 + 访问间接 + 构造复杂度。能不用就不用。

</details>
