# 附录 A：区域设置与大小写不敏感字符串比较

**Locales and Case-Insensitive String Comparisons**

## 本附录讲什么

大小写不敏感的字符串比较看似简单——`tolower(a) == tolower(b)`——但一旦涉及**区域设置（locale）**就变得复杂：不同 locale 下字母的大小写映射不同（如土耳其语 I→ı vs İ→i）。本附录讲 locale 对字符串比较的影响与可移植做法。

## 要点

### locale 影响大小写转换

`tolower(c)` 的行为依赖 `std::locale`。C locale（默认）下 `"İ"` 不存在；土耳其语 locale 下 `toupper('i')` 得 `İ`（U+0130）而非 `'I'`。跨 locale 的字符串比较可能产生不同结果——这是国际化软件的隐患。

### 标准库的不便

STL 没有内置的大小写不敏感 `string` 比较。常见做法：
```cpp
struct InsensitiveCmp {
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
            [](char x, char y){ return tolower((unsigned char)x) < tolower((unsigned char)y); });
    }
};
```

注意 `tolower` 要传 `unsigned char`（负 char 是 UB，见《C 和指针》ch9 ctype 陷阱）。

### HFT 实践

HFT 协议解析（FIX tag）默认用 **C locale**——ASCII 字段不涉及国际化，`tolower` 行为确定。但若处理多语言 symbol，要显式 `std::locale` 避免依赖全局 locale（全局 `setlocale` 非线程安全，见《C 和指针》ch16 locale）。

## 自测题

1. 为什么 `tolower(c)` 不能直接传 `char`？要传什么类型？
2. locale 如何影响大小写不敏感比较？全局 `setlocale` 有什么线程安全问题？
3. HFT 默认用哪个 locale？为什么？
