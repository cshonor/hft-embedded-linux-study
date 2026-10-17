# 条款 51：重载全局内存分配函数三思而后行

## 本节讲什么

全局替换 `new`/`delete` 影响整个程序所有内存分配，风险极高，优先类内局部重载。

## 示例

```cpp
// 全局 operator new/delete 影响整个程序，重载前三思
// 通常类专属 new/delete 或内存池更合适
```

---

## 代码自测

**题目 1：** 写了 placement new，为什么必须配套写 placement delete？
```cpp
class Widget {
    static void* operator new(std::size_t s, std::ostream& log) {
        log << "allocating";
        return ::operator new(s);
    }
    // 缺少什么？
};
```

<details>
<summary>参考答案</summary>

缺少配套的 placement delete：
```cpp
static void operator delete(void* p, std::ostream& log) {
    log << "freeing";
    ::operator delete(p);
}
```
如果 `Widget` 构造函数在 placement new 后抛异常，编译器需要调用对应签名的 placement delete 来释放内存。没有配套的 placement delete，内存泄漏。注意：placement delete 只在构造异常时被调用，正常析构走普通 delete。

</details>
