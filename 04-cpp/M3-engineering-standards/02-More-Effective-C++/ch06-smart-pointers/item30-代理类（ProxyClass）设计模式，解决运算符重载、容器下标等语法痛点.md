# 条款 30：代理类（Proxy Class）设计模式，解决运算符重载、容器下标等语法痛点

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Proxy {
    std::vector<int> &vec;
    std::size_t idx;
public:
    Proxy(std::vector<int> &v, std::size_t i) : vec(v), idx(i) {}
    int &operator*() { return vec[idx]; }
};
```

---

## 代码自测

**题目 1：** 代理类（Proxy Class）如何解决 `operator[]` 返回引用的问题？
```cpp
class CharArray {
    char* data;
public:
    char& operator[](int i) { return data[i]; }
};
CharArray arr;
arr[0] = 'a';  // OK
// 如果 data 是远程的（比如另一个进程），char& 无法工作
```

<details>
<summary>参考答案</summary>

如果数据不在本地内存（如远程、压缩存储），`char&` 无法直接引用。代理类方案：
```cpp
class CharProxy {
    CharArray& arr; int idx;
public:
    CharProxy(CharArray& a, int i) : arr(a), idx(i) {}
    operator char() const { return arr.read(idx); }  // 读取
    CharProxy& operator=(char c) { arr.write(idx, c); return *this; }
};
class CharArray {
public:
    CharProxy operator[](int i) { return CharProxy(*this, i); }
};
```
`arr[0] = 'a'` 先构造 `CharProxy`，然后调用 `operator=(char)`，实现延迟读写。

</details>
