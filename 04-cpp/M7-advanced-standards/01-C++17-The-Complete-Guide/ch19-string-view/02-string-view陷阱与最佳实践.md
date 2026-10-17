# 19.2 string_view 陷阱与最佳实践

> 第 19 章 字符串视图 · 上一节：[19.1 string_view 基础](01-string-view基础.md)

## 这节讲什么

string_view 不拥有数据——这是它最大的优势也是最大的陷阱。本节讲解悬垂 string_view、临时对象生命周期、以及最佳实践。

## 陷阱 1：悬垂 string_view

```cpp
// 返回 string_view 指向临时 string → 悬垂
std::string_view get_name() {
    std::string name = "hello";
    return name;  // 返回指向 name 的 string_view
    // name 在这里析构 → string_view 悬垂！
}

auto sv = get_name();
std::cout << sv;  // UB！sv 指向已释放的内存
```

### 安全的返回方式

```cpp
// 安全：返回 string（拥有数据）
std::string get_name() {
    return "hello";
}

// 安全：返回指向静态数据的 string_view
std::string_view get_literal() {
    return "hello";  // 字符串字面量有静态存储期
}

// 安全：返回成员的 string_view
class Config {
    std::string name_;
public:
    std::string_view name() const { return name_; }
    // 安全：name_ 的生命周期 >= Config 对象
};
```

## 陷阱 2：string_view 不以 \0 结尾

```cpp
std::string_view sv = "hello world";
auto sub = sv.substr(0, 5);  // "hello"

// sub.data() 返回 "hello" 的指针
// 但 sub.data() + sub.size() 不一定是 '\0'！
// 实际上 sub.data() 指向 "hello world"，sub.size()=5
// 所以 sub.data()[5] 是 ' '，不是 '\0'

// 危险：传给需要 \0 的 C 函数
printf("%s", sub.data());  // 输出 "hello world" 而不是 "hello"！

// 安全：转成 string
std::string s(sub);
printf("%s", s.c_str());  // 正确：输出 "hello"
```

## 陷阱 3：临时 string 转为 string_view

```cpp
// 隐式构造临时 string → string_view 指向临时 → 悬垂
std::string make_string();

void f(std::string_view sv);

f(make_string());  // 临时 string 在 f() 内有效，f() 返回后销毁
// 上面的代码是安全的：临时对象在完整表达式结束时销毁

// 但这样不安全：
std::string_view sv = make_string();  // 临时 string 在语句结束销毁
std::cout << sv;  // 悬垂！
```

## 陷阱 4：string_view 存入容器

```cpp
// 危险：vector 中的 string_view 可能悬垂
std::vector<std::string_view> views;

{
    std::string s = "hello";
    views.push_back(s);  // string_view 指向 s
}
// s 析构，views[0] 悬垂

// 安全：存 string（拥有数据）
std::vector<std::string> strings;
```

## 陷阱 5：string_view 参数和移动

```cpp
// 如果函数内部需要存储字符串，不要用 string_view 参数
class Config {
    std::string name_;
public:
    // 不好：可能多一次拷贝
    void set_name(std::string_view sv) {
        name_ = sv;  // 从 string_view 赋值 → 拷贝
    }

    // 好：按值传 string，可以移动
    void set_name(std::string name) {
        name_ = std::move(name);  // 移动
    }
};
```

## 最佳实践

### 1. 函数参数用 string_view（只读场景）

```cpp
// 好：只读参数用 string_view
void log_message(std::string_view msg);
int parse_int(std::string_view s);
bool validate(std::string_view symbol);
```

### 2. 返回值不要用 string_view（除非指向静态/成员数据）

```cpp
// 好：返回 string（拥有数据）
std::string format();

// 好：返回成员的 string_view
class Request {
    std::string path_;
public:
    std::string_view path() const { return path_; }
};

// 好：返回字面量
std::string_view default_name() { return "unknown"; }

// 坏：返回指向局部变量的 string_view
// std::string_view bad() { std::string s = "x"; return s; }
```

### 3. 不要存 string_view 做长期引用

```cpp
// 不好：类成员用 string_view
class Handler {
    std::string_view name_;  // 如果原始 string 销毁 → 悬垂
};

// 好：类成员用 string
class Handler {
    std::string name_;
};
```

### 4. 需要传给 C API 时转 string

```cpp
void c_api(const char* s);

void cpp_wrapper(std::string_view sv) {
    std::string s(sv);  // 确保 \0 终止
    c_api(s.c_str());
}
```

## HFT 关联

```cpp
// 解析行情消息——零拷贝
void on_message(std::string_view raw) {
    // 所有操作都是零拷贝视图
    auto type = raw.substr(0, 2);   // 消息类型
    auto body = raw.substr(2);       // 消息体

    // 解析 body 中的字段
    while (!body.empty()) {
        auto delim = body.find(',');
        auto field = body.substr(0, delim);
        process_field(field);  // 零拷贝传递
        body.remove_prefix(delim + 1);
    }
}

// 注意：raw 必须在 on_message 返回前有效
// 不要把 string_view 存到异步队列里
```

## 小结

| 陷阱 | 原因 | 解决 |
|------|------|------|
| 悬垂 string_view | 不拥有数据 | 确保原数据生命周期 |
| 不以 \0 结尾 | substr 后无 \0 | 转成 string 再传 C API |
| 存入容器 | 原数据可能销毁 | 存 string 而不是 string_view |
| 隐式构造临时 string | string_view 参数 | string_view 参数本身没问题，但不要绑定到临时 |

---

← [上一节](01-string-view基础.md) · [本章导读](./README.md)
