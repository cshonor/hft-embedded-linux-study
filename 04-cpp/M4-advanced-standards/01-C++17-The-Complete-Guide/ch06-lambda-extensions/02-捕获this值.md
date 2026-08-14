# 6.2 捕获 this 的值

> 第 6 章 Lambda 扩展 · 上一节：[6.1 constexpr lambda](01-constexpr-lambda.md) · 下一节：[6.3 泛型 lambda 与模板语法](03-泛型lambda与模板语法.md)

## 这节讲什么

C++17 之前，lambda 捕获 `this` 只能按引用（`[this]` 或 `[=]` 隐式捕获 this 引用）。如果 lambda 的生命周期超过对象——比如存入回调队列、异步任务——就会悬垂。C++17 允许 `[=*this]` 按值拷贝整个对象。

## C++14 的痛点

```cpp
class Worker {
    int id_;
public:
    Worker(int id) : id_(id) {}

    auto get_callback() {
        // C++14：[this] 捕获 this 指针（引用）
        return [this]() {
            std::cout << "Worker " << id_ << "\n";  // 访问 id_ 需要此
        };
    }
};

Worker w(1);
auto cb = w.get_callback();
w.~Worker();  // w 销毁，this 失效
cb();         // UB！访问已销毁对象的 id_
```

### `[=]` 隐式捕获 this 的问题

```cpp
auto get_callback() {
    return [=]() {           // C++14：[=] 隐含 [this]（引用捕获）
        std::cout << id_;    // 实际通过 this->id_
    };
}
// 同样的问题：this 是引用，对象销毁后悬垂
```

## C++17 的 `[*this]`

```cpp
class Worker {
    int id_;
public:
    Worker(int id) : id_(id) {}

    auto get_callback() {
        // C++17：[*this] 拷贝整个对象到 lambda
        return [*this]() {
            std::cout << "Worker " << id_ << "\n";  // id_ 是拷贝的
        };
    }
};

Worker w(1);
auto cb = w.get_callback();
// w 销毁后 cb 仍然安全——它有自己的副本
cb();  // ✅ 输出 "Worker 1"
```

## 按值捕获 vs 按引用捕获

```cpp
class Worker {
    int id_;
public:
    auto by_ref()   { return [this]()   { return id_; }; }   // 引用
    auto by_val()   { return [*this]()  { return id_; }; }   // 值拷贝
    auto by_val_ref { return [=]()      { return id_; }; }   // C++14: 引用(隐式 this)
};
```

| 捕获方式 | 语法 | 行为 | 生命周期 |
|---------|------|------|---------|
| 引用 | `[this]` | 通过 this 指针访问成员 | 依赖对象存活 |
| 隐式引用 | `[=]` | C++14 隐含 `[this]` | 依赖对象存活 |
| 值 | `[*this]` | 拷贝整个对象到 lambda | 独立副本 |

## 实际用法

### 异步回调

```cpp
class Config {
    std::string host_;
    int port_;
public:
    Config(std::string h, int p) : host_(h), port_(p) {}

    void async_connect() {
        // C++17：拷贝 config 到回调，即使 config 析构也安全
        post_async([*this]() {
            return connect(host_, port_);  // 用自己的副本
        });
    }
};
```

### 线程池任务

```cpp
class Strategy {
    Params params_;
public:
    void submit_backtest() {
        thread_pool.submit([*this]() {
            // 即使 Strategy 对象被销毁，回测仍然安全
            run_backtest(params_);
        });
    }
};
```

## `[*this]` 的代价

```cpp
// 拷贝整个对象
class BigConfig {
    std::array<char, 4096> data_;
public:
    auto get_cb() {
        return [*this]() { /* ... */ };  // 拷贝 4KB 到 lambda！
    }
};
// 如果对象大，[*this] 代价高
```

**规则**：小对象用 `[*this]` 安全；大对象考虑 `shared_ptr` 共享所有权。

### shared_ptr 替代方案

```cpp
class BigWorker : public std::enable_shared_from_this<BigWorker> {
    std::array<char, 4096> data_;
public:
    auto get_cb() {
        auto self = shared_from_this();  // 引用计数 +1
        return [self]() { /* 用 self->data_ */ };
        // 不拷贝数据，只拷贝 shared_ptr（8 字节）
    }
};
```

## C++20 的改进

C++20 中 `[=]` 不再隐式捕获 `this`（弃用警告）：

```cpp
// C++20：[=] 不隐含 [this]，需要显式写
auto cb = [=, this]() { return id_; };  // C++20：显式
auto cb = [=]() { return id_; };        // C++20：警告（隐式捕获 this 已弃用）
```

## HFT 关联

```cpp
// 策略引擎注册回调
class StrategyEngine {
    Config config_;
public:
    void register_callbacks() {
        // 行情回调：拷贝 config，防止引擎销毁后回调悬垂
        market_data_.on_tick([*this](const Tick& t) {
            if (t.price > config_.threshold) {
                execute(t);
            }
        });

        // 定时器回调
        timer_.on_timeout([*this]() {
            check_risk(config_);
        });
    }
};
```

## 小结

| 特性 | C++14 | C++17 |
|------|-------|-------|
| `[this]`（引用） | ✅ | ✅ |
| `[*this]`（值） | ❌ | ✅ |
| `[=]` 隐式捕获 this | ✅ | ✅（C++20 弃用） |
| 安全回调 | 需要 shared_ptr | `[*this]` |

---

← [上一节](01-constexpr-lambda.md) · [下一节 →](03-泛型lambda与模板语法.md)
