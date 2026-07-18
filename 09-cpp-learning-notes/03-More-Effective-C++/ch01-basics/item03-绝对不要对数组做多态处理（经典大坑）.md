# 条款 3：绝对不要对数组做多态处理（经典大坑）

## 本节讲什么

多态允许通过基类指针或引用操作派生类对象，但**绝不能把多态用在数组上**。用基类指针遍历或 `delete[]` 派生类数组，程序几乎不可能按预期运行——这是 C++ 中经典且后果严重的大坑。本条款说明两处核心原因，并给出规避思路。

## 核心结论

**指针算术与多态不能混用；数组操作深度依赖指针算术，因此数组与多态绝对不能用在一起。**

## 原因 1：隐藏的指针算术导致寻址错误

`array[i]` 本质是 `*(array + i)`。编译器生成遍历代码时，用 `i * sizeof(对象大小)` 计算偏移。

若函数参数是**基类指针**，编译器按 **`sizeof(Base)`** 步进；而派生类通常有额外成员，**对象往往比基类更大**。把派生类数组当基类数组传入时，指针按「较短」的基类步长前进，会指向错误地址，产生不可预知的灾难性后果。

```cpp
class Base {
public:
    virtual ~Base() = default;
    int b;
};

class Derived : public Base {
public:
    int extra;  // 派生类比基类更大
};

void process(Base arr[], int n) {
    for (int i = 0; i < n; ++i) {
        // arr[i] 等价 *(arr + i)，步长为 sizeof(Base)
        // 若实际传入 Derived[]，第 i 个元素地址算错
        (void)arr[i];
    }
}

void bad() {
    Derived darr[10];
    process(darr, 10);  // 危险：按 sizeof(Base) 而非 sizeof(Derived) 步进
}
```

## 原因 2：通过基类指针 delete[] 派生类数组

`delete[]` 时，编译器须**按构造相反顺序**对每个元素调用析构函数。若通过基类指针删除派生类数组，编译器仍按 **`sizeof(Base)`** 计算元素地址，析构循环无法正确运行。

C++ 标准明确规定：**通过基类指针删除含有派生类对象的数组，结果是不确定的（undefined）**。

```cpp
void leak_or_crash() {
    Derived *parr = new Derived[10];
    Base *bp = parr;
    delete[] bp;  // 未定义行为：步长与析构顺序均错误
}
```

## 正确做法

- **不要**把「派生类数组」以基类指针/引用形式传递或遍历。
- 若需多态容器，用 **`std::vector<std::unique_ptr<Base>>`** 等，每个元素独立分配，步长问题不存在。
- 若必须持有同质对象数组，数组元素类型应一致（全是 `Derived`，或不用多态数组）。

```cpp
#include <memory>
#include <vector>

void good() {
    std::vector<std::unique_ptr<Base>> vec;
    vec.push_back(std::make_unique<Derived>());
    // 每个对象独立堆分配，无 sizeof 步进陷阱
}
```

## 与 Item 33 的关系

为避免不经意写出多态数组，可参考 [条款 33](../ch07-miscellany/item33-把非叶子类设计为抽象类，强制约束子类实现接口，架构约束.md)：**把非叶子类设计为抽象类**。若避免「从一个具体类派生出另一个具体类」，就不太容易在代码里误用多态数组。

## 示例：错误 vs 安全

```cpp
class Base { public: virtual ~Base() = default; };
class Derived : public Base {};

// 错误 1：派生类数组当基类数组用
// Derived darr[10];
// void f(Base *p, int n);
// f(darr, 10);

// 错误 2：基类指针 delete[] 派生类数组
// Derived *p = new Derived[10];
// delete[] static_cast<Base *>(p);

// 安全：多态用指针容器，而非对象数组
// std::vector<std::unique_ptr<Base>> items;
```

## 小结

- 数组索引 = 指针算术；多态下步长按**静态类型**（基类）算，与**实际对象大小**（派生类）不一致 → 寻址与析构皆错。
- **永远不要**用基类指针遍历或 `delete[]` 派生类数组。
- 多态集合优先用「指针/智能指针的容器」，架构上可用抽象非叶子类降低误用概率。
