# 4.2 多重继承的 vtable 与 this 调整

> 第 4 章 · 上一节：[4.1 虚函数分派](01-vtable-dispatch.md) · 下一节：[4.3 虚基类下的虚函数](03-virtual-base-vfunc.md)

## 这节讲什么

多重继承下派生类有多个 vptr。调非首基类的虚函数时，`this` 要调整到对应基类子对象——thunk 技术在 vtable 里插入调整代码，让多继承的虚函数调用比单继承多一次 this 调整。

---

## 为什么要学这个（先建立直觉）

C 程序员模拟多继承时手动处理指针偏移：

```c
// C 的"多继承"
struct A_C { int a; };
struct B_C { int b; };
struct C_C {
    struct A_C a_part;
    struct B_C b_part;
    int c;
};
// 访问 B 的成员：obj.b_part.b
// 把 C* 转成 B*：B_ptr = &obj.b_part（手动偏移）
```

C++ 的多继承让编译器自动处理 this 调整，但虚函数调用时需要 thunk：

```cpp
class A { public: virtual void fa(); int a; };
class B { public: virtual void fb(); int b; };
class C : public A, public B { int c; };
// C 的内存：[A:vptr|a | B:vptr|b | c]
C* c = new C;
B* b = c;  // 编译器自动偏移：b = (char*)c + sizeof(A子对象)
b->fb();   // 调 B 的虚函数，但实际对象是 C
// vtable 里的 thunk 先把 this 从 B* 调整回 C*，再调 C::fb
```

---

## this 调整与 thunk 详解

### 内存布局

```cpp
class A { public: virtual void fa(); int a; };  // [vptr|a] = 16B
class B { public: virtual void fb(); int b; };  // [vptr|b] = 16B
class C : public A, public B { public: int c; };
// C 的内存布局：
// [A vptr | a | B vptr | b | c | padding]
//   ↑ C* 指向这里     ↑ B* 指向这里
//   sizeof(C) = 40+
```

### thunk 的工作

```cpp
C* c = new C;
B* b = c;       // b = (char*)c + 16（偏移到 B 子对象）
b->fb();        // 调 B 的虚函数
// 如果 C override 了 fb：
// vtable[fb] 指向的不是 C::fb 的地址，而是一个 thunk：
// thunk: this -= 16; jmp C::fb
// thunk 先把 this 从 B* 调整回 C*，再跳转到 C::fb
```

### 单继承 vs 多继承

```cpp
// 单继承：复用一个 vptr，无需 this 调整
class Single : public A { int x; };
// [A vptr | a | x]  ← 只有一个 vptr

// 多继承：多个 vptr，需要 this 调整
class Multi : public A, public B { int x; };
// [A vptr | a | B vptr | b | x]  ← 两个 vptr
// 调 B 的虚函数时 this 要调整
```

---

## 常见错误（新手踩坑）

### 错误 1：void* 转多继承指针

```cpp
class A { public: virtual void f(); };
class B { public: virtual void g(); };
class C : public A, public B {};
C* c = new C;
void* v = c;
B* b = static_cast<B*>(v);  // UB！v 指向 C 的起始，不是 B 子对象
// 正确：B* b = c;（编译器自动偏移）
```

### 错误 2：多继承的 sizeof 暴涨

```cpp
class A { virtual void f(); int a; };  // 16B
class B { virtual void g(); int b; };  // 16B
class C : public A, public B { int c; }; // 40B
// 两个 vptr + padding → 比预期大很多
```

### 错误 3：钻石继承数据重复

```cpp
class A { int data; };
class B : public A {};
class C : public A {};
class D : public B, public C {};
// D 有两份 A::data → 歧义
// 修正：虚继承
```

---

## 和 C 的区别

| 特性 | C 嵌套结构体 | C++ 多继承 |
|------|-------------|-----------|
| 指针偏移 | 手动 `&obj.b_part` | 编译器自动 `static_cast<B*>(c)` |
| 虚函数 | N/A | vtable + thunk（this 调整） |
| 开销 | 无隐藏开销 | 多个 vptr + thunk 调整 |

---

## HFT 关联

1. **避免多继承热路径**：多继承的 this 调整 + 多个 vptr 让对象膨胀 + 调用变慢。用组合替代。
2. **sizeof 膨胀**：多继承每个有虚函数的基类贡献一个 vptr——对象可能膨胀 16+ 字节，破坏 cache 利用率。
3. **组合优于多继承**：`struct Engine { MarketData md; OrderBook ob; }` 无 this 调整，sizeof 可控。

---

## 代码自测

### Q1: 地址偏移

```cpp
class A { public: virtual void f(); int a; };
class B { public: virtual void g(); int b; };
class C : public A, public B { int c; };
C* c = new C;
A* a = c;
B* b = c;
// a == c 吗？b == c 吗？
```

<details>
<summary>答案与复习指引</summary>

`a == c`（A 是首基类，地址相同）。`b != c`（B 是第二个基类，b 偏移到 B 子对象位置）。编译器自动做 this 调整。

**复习：** → [4.2 多继承的 vtable](./02-multi-inheritance-vtable.md)
</details>

### Q2: thunk 作用

```cpp
class A { public: virtual void f() { cout << "A"; } };
class B { public: virtual void g() { cout << "B"; } };
class C : public A, public B {
public:
    void g() override { cout << "C"; }  // override B::g
};
B* b = new C;
b->g();  // 输出什么？this 如何调整？
```

<details>
<summary>答案与复习指引</summary>

输出 `C`。`b` 指向 B 子对象，`b->g()` 经 B 的 vtable 找到 `C::g` 的 thunk。thunk 先把 `this` 从 B 子对象偏移回 C 的起始，再调 `C::g()`。比单继承多一次 this 调整。

**复习：** → [4.2 多继承的 vtable](./02-multi-inheritance-vtable.md)
</details>

### Q3: void* 转换

```cpp
class A { public: virtual void f(); };
class B { public: virtual void g(); };
class C : public A, public B {};
C* c = new C;
void* v = c;
// 以下哪个安全？
// A: B* b = static_cast<B*>(v);
// B: B* b = dynamic_cast<B*>(static_cast<A*>(v));
// C: B* b = c;
```

<details>
<summary>答案与复习指引</summary>

C 安全。`B* b = c` 让编译器自动做 this 调整。A 不安全（`static_cast<B*>(void*)` 不做偏移）。B 也不安全（先转 A* 没问题，但 dynamic_cast 需要 RTTI 且可能失败）。多继承指针转换必须让编译器知道源类型。

**复习：** → [4.2 多继承的 vtable](./02-multi-inheritance-vtable.md)
</details>

### Q4: 组合替代

```cpp
// 多继承方案
class FastEngine : public MarketData, public OrderBook, public Risk {};

// 组合方案
struct FastEngine {
    MarketData md;
    OrderBook ob;
    Risk risk;
};
// 哪个更适合 HFT？为什么？
```

<details>
<summary>答案与复习指引</summary>

组合方案。多继承引入多个 vptr（如果基类有虚函数）+ this 调整开销 + sizeof 暴涨。组合的 sizeof 可预测，无 this 调整，cache 友好。HFT 数据结构优先组合。

**复习：** → [4.2 多继承的 vtable](./02-multi-inheritance-vtable.md)
</details>

---

## 参考与延伸

- 下一节：[4.3 虚基类下的虚函数](03-virtual-base-vfunc.md)
- 回到：[第 4 章 函数语义](README.md)
