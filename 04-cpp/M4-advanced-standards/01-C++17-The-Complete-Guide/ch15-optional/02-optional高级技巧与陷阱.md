# 15.2 optional 高级技巧与陷阱

> 第 15 章 std::optional · 上一节：[15.1 optional 基础用法](01-optional基础用法.md)

## 这节讲什么

本节深入 optional 的陷阱：移动语义、引用类型、异常安全、性能考虑，以及实际工程中的最佳实践。

## 移动语义

```cpp
std::optional<std::vector<int>> make() {
    std::vector<int> v(1000);
    return v;  // 拷贝或移动
}

auto opt = make();  // opt 持有 vector

// 从 optional 中移出
std::vector<int> data = std::move(*opt);
// opt 仍然 has_value()，但内部 vector 已被移动（空）
opt.reset();  // 显式清空
```

### 陷阱：移动后 optional 仍有值

```cpp
std::optional<std::string> opt = "hello";
auto s = std::move(*opt);
// opt.has_value() 仍然是 true！
// 但 *opt 是空的 string（已被移动）
std::cout << *opt;  // 输出空字符串

// 要真正清空，用 reset
opt.reset();
```

## optional 的引用？

C++17 的 `optional` 不能直接持有引用：

```cpp
// C++17：不支持
// std::optional<int&> opt = x;  // 编译错误

// 变通：用 optional<int*> 或 optional<reference_wrapper<int>>
int x = 42;
std::optional<std::reference_wrapper<int>> opt = std::ref(x);
opt->get() = 100;  // x = 100
```

C++26 可能支持 `optional<T&>`。

## 异常安全

### value() 抛异常

```cpp
std::optional<int> opt;
try {
    int x = opt.value();  // 抛 std::bad_optional_access
} catch (const std::bad_optional_access& e) {
    std::cout << "No value: " << e.what() << "\n";
}

// HFT 中不要用 value()——异常开销大
// 用 *opt（先检查 has_value()）或 value_or()
```

### emplace 的异常安全

```cpp
std::optional<BigObject> opt;
opt.emplace(args...);  // 如果构造函数抛异常，opt 保持空
// 不会留下半构造的对象
```

## 性能考虑

### optional 的额外开销

```cpp
// sizeof(optional<T>) = sizeof(T) + 1（标志位）+ padding
// 比直接 T 多 1-7 字节

// 栈上存储：不动态分配
std::optional<int> opt;  // 全在栈上

// 拷贝开销：需要拷贝 T + 标志位
// 移动开销：同上
```

### 热路径上避免频繁构造/析构

```cpp
// 不好：每次调用都构造 optional
std::optional<Tick> get_tick() {
    return Tick{...};  // 每次都构造 optional
}

// 好：热路径直接返回值，用哨兵值表示无效
struct TickResult {
    bool valid;
    Tick tick;
};
TickResult get_tick() { ... }
// 避免 optional 的额外开销
```

## 最佳实践

### 1. 默认用 value_or()

```cpp
// 好：简洁安全
int port = config.get("port").value_or(8080);

// 不好：啰嗦
int port;
if (auto p = config.get("port")) {
    port = *p;
} else {
    port = 8080;
}
```

### 2. 不要忽略空检查

```cpp
// 不好：可能 UB
auto opt = find();
auto x = *opt;  // 如果 opt 空 → UB

// 好：先检查
if (auto opt = find(); opt) {
    auto x = *opt;
}
```

### 3. optional 作为参数

```cpp
// 用 optional 表示"可选参数"
void configure(std::optional<int> port = std::nullopt,
               std::optional<std::string> host = std::nullopt);

configure(8080, "localhost");
configure(std::nullopt, "localhost");  // 只设 host
configure(8080);                       // 只设 port
```

### 4. optional 成员变量

```cpp
class Engine {
    std::optional<Config> config_;  // 延迟初始化
public:
    void init(const Config& cfg) { config_ = cfg; }
    void run() {
        if (!config_) throw std::runtime_error("not initialized");
        // 用 *config_
    }
};
```

## HFT 关联

```cpp
// optional 用于延迟初始化和可空字段
struct OrderBook {
    std::optional<Level> best_bid_;  // 初始可能没有 bid
    std::optional<Level> best_ask_;

    void update(const Tick& t) {
        if (t.side == Side::BUY) best_bid_ = Level{t.price, t.qty};
        else best_ask_ = Level{t.price, t.qty};
    }

    double spread() const {
        if (best_bid_ && best_ask_) {
            return best_ask_->price - best_bid_->price;
        }
        return 0;  // 一边不存在，spread 为 0
    }
};
```

## 小结

| 陷阱 | 说明 |
|------|------|
| 移动后仍有值 | `std::move(*opt)` 后 has_value() 仍为 true |
| 不支持引用 | 用 `optional<reference_wrapper<T>>` 变通 |
| value() 抛异常 | 热路径用 `*opt` + `has_value()` |
| 额外开销 | 多 1 字节标志位 + padding |

---

← [上一节](01-optional基础用法.md) · [本章导读](./README.md)
