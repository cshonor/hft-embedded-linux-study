# 条款 26：尽可能延后变量定义

## 本节讲什么

缩小变量作用域，减少不必要构造/析构，只在真正要用的时候再定义。

## 示例

```cpp
std::string process() {
    if (condition)
        std::string s;  // 需要时才定义，缩小作用域
        // ...
    return result;
}
```

---

## 代码自测

**题目 1：** 以下两种写法哪种更好？为什么？
```cpp
// 方式A：循环外定义
string s;
for (int i = 0; i < n; i++) {
    s = f(i);
    use(s);
}
// 方式B：循环内定义
for (int i = 0; i < n; i++) {
    string s = f(i);
    use(s);
}
```

<details>
<summary>参考答案</summary>

方式B更好（通常）。方式A虽然只构造一次，但每次赋值可能仍有开销（如果 f(i) 返回的 string 很长），且 s 的作用域过大——循环结束后仍可见，可能被误用。方式B每次构造+析构，但：1) 作用域最小化更安全；2) 如果循环中有 continue/return，方式B不会浪费已构造的对象；3) 现代 C++ 编译器通常能优化掉多余构造。

</details>
