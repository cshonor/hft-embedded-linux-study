# 条款 50：写了 placement new，一定要配套写 placement delete

## 本节讲什么

构造抛异常时，编译器会调用对应版本 placement delete 回收内存，只写 new 不写 delete 会内存泄漏。

## 示例

```cpp
void *operator new(std::size_t, void *place) { return place; }
void operator delete(void *, void *) noexcept {}  // placement delete
```

---

## 代码自测

**题目 1：** 替换 `operator new` 时，为什么要处理 size==0 的情况？
```cpp
void* operator new(std::size_t size) {
    if (size == 0) size = 1;  // 为什么？
    while (true) {
        void* p = malloc(size);
        if (p) return p;
        // new-handler 处理...
    }
}
```

<details>
<summary>参考答案</summary>

C++ 标准允许 `new T` 中 `T` 是空类（`sizeof(T) == 0`），但 `operator new` 必须返回合法指针。如果不处理 `size==0`，`malloc(0)` 的行为是实现定义的（可能返回 nullptr 或返回不可用的指针）。统一处理为 `size = 1` 保证返回有效内存。同时 `operator delete` 也要能处理来自 `new(0)` 的指针。

</details>
