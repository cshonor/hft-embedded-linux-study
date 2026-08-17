# 条款 28：不要返回指向对象内部成员的句柄（指针/引用）

## 本节讲什么

外部拿到内部成员引用，可以绕过 `private` 修改内部状态，破坏封装；临时对象引用还会悬垂。

## 示例

```cpp
class String {
    char *data;
public:
    const char &operator[](int i) const { return data[i]; }
    // 不要返回 char& 的非 const 版本给 const 对象外的句柄
};
```

---

## 代码自测

**题目 1：** 下面代码返回内部成员的引用有什么问题？
```cpp
class Rectangle {
    Rect r;
public:
    const Rect& getRect() const { return r; }
};
const Rect* p;
{
    Rectangle rect;
    p = &rect.getRect();
}  // rect 析构
p->area();  // 安全吗？
```

<details>
<summary>参考答案</summary>

不安全。`rect` 析构后 `r` 也被销毁，`p` 成为悬空指针。返回指向对象内部的句柄（指针/引用/迭代器）会削弱封装性，并存在生命周期风险——即使返回 const 引用，调用方仍可能持有超出对象生命周期的句柄。

</details>
