# 条款 23：优先用非成员、非友元函数替代成员函数

## 本节讲什么

扩大函数扩展空间，不破坏类封装，STL 算法大多是非成员函数。

## 示例

```cpp
class UDate {
    friend bool checkValidity(const UDate &);
public:
    int month() const;
};
bool checkValidity(const UDate &d) { return d.month() <= 12; }
```

---

## 代码自测

**题目 1：** 下面哪种写法更好？为什么？
```cpp
// 方式A：成员函数
class WebBrowser {
public:
    void clearEverything();  // 清缓存+历史+cookie
};
// 方式B：非成员函数
void clearBrowser(WebBrowser& wb) {
    wb.clearCache();
    wb.clearHistory();
    wb.clearCookies();
}
```

<details>
<summary>参考答案</summary>

方式B更好。非成员非友元函数不增加类成员，不破坏封装性——它只能通过 public 接口操作。同时更符合「包弹性」：可以把 `clearBrowser` 放在不同头文件中，按功能分区（如 `webbrowser_cache.h`、`webbrowser_history.h`）。

</details>
