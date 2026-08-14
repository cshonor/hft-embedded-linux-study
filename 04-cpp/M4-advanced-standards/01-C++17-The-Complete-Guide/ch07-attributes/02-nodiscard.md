# 7.2 [[nodiscard]]

> 第 7 章 新属性与属性扩展 · 上一节：[7.1 [[maybe_unused]] 与 [[fallthrough]]](01-maybe_unused与fallthrough.md) · 下一节：[7.3 属性命名空间与 using](03-属性命名空间与using.md)

## 这节讲什么

`[[nodiscard]]` 告诉编译器：这个函数的返回值不能被忽略。如果调用者丢弃了返回值，编译器发出警告。这对防止"忘记检查错误码"类 bug 非常有用。

## 基本用法

```cpp
[[nodiscard]] int compute_important() {
    return 42;
}

compute_important();  // 警告：返回值被丢弃
int x = compute_important();  // OK：返回值被使用
```

## 典型场景

### 1. 错误码

```cpp
[[nodiscard]] Status send_order(const Order& ord);

send_order(ord);  // 警告！你没检查返回值
if (send_order(ord) == Status::OK) { ... }  // OK
```

### 2. 工厂函数

```cpp
[[nodiscard]] std::unique_ptr<Connection> connect(const char* addr);

connect("localhost:8080");  // 警告！创建了连接但没持有 → 内存泄漏
auto conn = connect("localhost:8080");  // OK
```

### 3. 只读查询

```cpp
class OrderBook {
public:
    [[nodiscard]] double best_bid() const;
    [[nodiscard]] int total_orders() const;
};

book.best_bid();  // 警告！调用了查询但没用结果
double price = book.best_bid();  // OK
```

### 4. 返回值移动语义

```cpp
[[nodiscard]] std::string generate_id();

generate_id();  // 警告！生成的 ID 被丢弃
auto id = generate_id();  // OK
```

## 在类型上使用（C++20）

```cpp
// C++20：[[nodiscard]] 可以加在类型上
struct [[nodiscard]] ErrorCode {
    int code;
    const char* msg;
};

ErrorCode send(const char* data);

send("hello");  // 警告！ErrorCode 被丢弃
ErrorCode ret = send("hello");  // OK
```

C++17 只能加在函数上，C++20 扩展到类型。

## 在函数指针/引用上的行为

```cpp
[[nodiscard]] int important();

auto f = important;  // OK：取函数地址，不是调用
f();                 // 警告？取决于编译器实现
// 注意：通过函数指针调用，nodiscard 可能不生效
```

## 强制检查的惯用法

```cpp
// 返回一个必须检查的结果类型
[[nodiscard]] Result<bool, Error> try_parse(const std::string& s);

auto r = try_parse("123");
if (r.has_value()) {
    use(r.value());
} else {
    handle_error(r.error());
}

// 忘记检查 → 警告
try_parse("123");  // 警告！
```

## HFT 关联

```cpp
class RiskManager {
public:
    // 风控检查结果不能忽略
    [[nodiscard]] bool check_order(const Order& ord) const {
        return ord.qty <= max_qty_ && ord.price > 0;
    }

    // 发送结果不能忽略
    [[nodiscard]] SendStatus send(const Order& ord);
};

// 使用
if (!risk_.check_order(ord)) {
    reject(ord);
    return;
}
// risk_.check_order(ord);  // 警告！没检查结果 → 防止风控被绕过
```

## 标准库中的 [[nodiscard]]

C++17 起标准库大量使用 `[[nodiscard]]`：

```cpp
// std::empty
[[nodiscard]] constexpr bool empty(const T& c);

// std::move
[[nodiscard]] constexpr remove_reference_t<T>&& move(T&& t) noexcept;

// std::forward
[[nodiscard]] constexpr T&& forward(remove_reference_t<T>& t) noexcept;

// allocator::allocate
[[nodiscard]] T* allocate(std::size_t n);
```

```cpp
std::vector<int> v;
std::empty(v);  // 警告！返回值被丢弃

std::move(v);   // 警告！move 的结果被丢弃（v 没变，只是返回了一个 xvalue）
auto v2 = std::move(v);  // OK
```

## 小结

| 场景 | 不用 nodiscard | 用 nodiscard |
|------|---------------|-------------|
| 忘记检查错误码 | 静默通过 | 编译警告 |
| 忘记持有资源 | 内存泄漏 | 编译警告 |
| 忘记用查询结果 | 逻辑错误 | 编译警告 |

---

← [上一节](01-maybe_unused与fallthrough.md) · [下一节 →](03-属性命名空间与using.md)
