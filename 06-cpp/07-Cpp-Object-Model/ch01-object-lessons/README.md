# 第 1 章 关于对象

**Object Lessons**

## 本章讲什么

C++ 对象在内存里到底长什么样？本章给出 C++ 对象模型的全景：成员变量如何布局、成员函数放在哪、虚函数的 `vptr`/`vtable` 机制、继承下的对象大小与布局。理解这些才能预测 `sizeof`、cache 行为、虚函数的间接代价。

## 要点

### 对象模型的三个规则

1. **非静态数据成员**：在对象内部（占用对象空间）。
2. **静态数据成员**：在对象外部（全局/静态区，唯一一份）。
3. **成员函数**（静态/非静态）：在对象外部（代码段），对象只存数据。

```cpp
class Point { int x, y; static int count; void draw(); };
// sizeof(Point) = 8（两个 int），不含 count 和 draw
```

### 虚函数与 vptr

有虚函数的类，对象内多一个 `vptr`（虚表指针，8 字节）指向类的 `vtable`（虚函数表）。`vtable` 是函数指针数组，存各虚函数的实际地址。

```cpp
class Shape { public: virtual void draw(); virtual double area(); };
// sizeof(Shape) = 8（vptr），draw/area 在 vtable 里
```

虚函数调用 = 经 `vptr` 间接取 `vtable[slot]` 再 call——比普通函数多一次间接访存（cache miss 代价）。

### 继承布局

| 继承 | 布局 | 代价 |
|------|------|------|
| 单继承 | 派生成员追加在基类之后 | 一个 vptr |
| 多继承 | 多个基类子对象，多个 vptr | this 调整 |
| 虚继承 | 虚基类指针/偏移表 | 额外间接 |

### 封装的代价

C++ 的封装（`private`/`public`）**运行时零开销**——访问控制是编译期检查。真正的代价来自：虚函数的 vtable 间接、虚基类的 this 调整、多继承的布局膨胀。

## HFT 关联

- **热路径禁虚函数**：虚函数的 vtable 间接在每 tick 路径上引入 cache miss + 分支预测代价。HFT 策略分派用 `enum` + `switch`/函数指针数组（jump table）或模板静态分派，避免虚函数。
- **`sizeof` 与 cache 行**：对象大小直接影响一个 cache 行能装几个对象。`vptr` 让对象多 8 字节，可能让每行对象数减半——`vector<Order>` 遍历的 cache 命中率受影响。
- **POD vs 非 POD**：无虚函数、无自定义构造的 POD 类型可用 `memcpy` 且 cache 友好；有 vptr 的非 POD 不能 `memcpy`（会破坏 vptr）。HFT 热路径数据结构尽量 POD。

## 自测题

1. `sizeof(Point)`（两个 int + 一个静态 int + 一个成员函数）是多少？为什么不含静态成员和函数？
2. vptr/vtable 的工作机制是什么？虚函数调用比普通函数多什么代价？
3. 单继承、多继承、虚继承的对象布局分别有什么代价？
4. C++ 的 `private`/`public` 访问控制有运行时开销吗？真正的封装代价来自哪里？
5. HFT 热路径为什么避免虚函数？用什么替代？
