# 条款 28：手写基础版智能指针，理解智能指针的核心逻辑

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
template<typename T>
class SmartPtr {
    T *ptr;
public:
    explicit SmartPtr(T *p) : ptr(p) {}
    ~SmartPtr() { delete ptr; }
    T &operator*() { return *ptr; }
};
```

---

## 代码自测

**题目 1：** 手写智能指针需要实现哪些核心功能？
```cpp
template<typename T>
class SmartPtr {
    T* ptr;
    // 还需要什么？
};
```

<details>
<summary>参考答案</summary>

核心功能：1) 构造（接受裸指针）、析构（delete）；2) 拷贝构造和赋值（需要决定语义：转移 vs 共享 vs 禁止）；3) `operator*` 和 `operator->`（解引用）；4) `get()`（返回原始指针）；5) `operator bool`（判空）；6) `release()`（放弃所有权）；7) `reset()`（释放旧资源，接管新资源）。转移语义版（类似 unique_ptr）拷贝构造和赋值应被禁止或实现为移动。

</details>
