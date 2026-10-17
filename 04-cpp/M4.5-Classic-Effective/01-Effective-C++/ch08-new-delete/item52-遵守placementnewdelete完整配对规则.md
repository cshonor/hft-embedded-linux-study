# 条款 52：遵守 placement new/delete 完整配对规则

## 本节讲什么

不同参数签名的分配释放函数必须一一对应。

## 示例

```cpp
alignas(std::max_align_t) char buf[sizeof(T)];
T *p = new (buf) T(args);
// ...
p->~T();  // placement new 不自动析构
```

---

## 代码自测

**题目 1：** placement new/delete 的名字遮蔽问题如何避免？
```cpp
class Base {
public:
    static void* operator new(std::size_t, void* p) { return p; }
};
class Derived : public Base {
public:
    static void* operator new(std::size_t s) { return malloc(s); }
    // Base::operator new(size, void*) 还能用吗？
};
```

<details>
<summary>参考答案</summary>

不能用。`Derived` 中的 `operator new(size_t)` 遮蔽了 `Base` 中的 `operator new(size_t, void*)`。修复：在 Derived 中用 using 声明引入基类版本：
```cpp
class Derived : public Base {
public:
    using Base::operator new;  // 引入 placement new
    static void* operator new(std::size_t s) { return malloc(s); }
};
```
或确保不自定义普通 new（让编译器使用全局的）。

</details>
