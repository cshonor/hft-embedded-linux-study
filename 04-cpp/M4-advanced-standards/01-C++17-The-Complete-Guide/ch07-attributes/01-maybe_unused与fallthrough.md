# 7.1 [[maybe_unused]] 与 [[fallthrough]]

> 第 7 章 新属性与属性扩展 · 下一节：[7.2 [[nodiscard]]](02-nodiscard.md)

## 这节讲什么

C++17 新增三个标准属性：`[[maybe_unused]]`、`[[fallthrough]]`、`[[nodiscard]]`。本节讲前两个——消除"未使用变量"和"switch 穿透"的编译器警告。

## [[maybe_unused]]

### 问题

```cpp
int process(int data, int /*unused*/) {
    // 第二个参数在当前版本没用，但接口要求保留
    return data * 2;
    // 编译器警告：unused parameter
}
```

### C++17 方案

```cpp
int process(int data, [[maybe_unused]] int reserved) {
    return data * 2;
    // 无警告
}
```

### 适用位置

```cpp
// 1. 未使用的变量
[[maybe_unused]] int debug_counter = 0;

// 2. 未使用的函数参数
void callback([[maybe_unused]] int event, int data) {
    process(data);
}

// 3. 未使用的函数
[[maybe_unused]] static void helper() { /* debug only */ }

// 4. 未使用的类型别名
[[maybe_unused]] using LargeInt = long long;

// 5. 未使用的 lambda 捕获
auto f = [x, [[maybe_unused]] y](int v) { return v + x; };
```

### 对比老方法

```cpp
// C：(void)cast
void callback(int event, int data) {
    (void)event;  // 老方法：cast 到 void
    process(data);
}

// C++17：更清晰
void callback([[maybe_unused]] int event, int data) {
    process(data);
}
```

## [[fallthrough]]

### 问题：switch 穿透

```cpp
switch (status) {
    case Status::OK:
        do_ok();
        // 忘了 break！穿透到下一个 case
    case Status::WARN:
        do_warn();
        break;
    // 编译器可能警告：可能忘记 break
}
```

### 有意的穿透

```cpp
switch (level) {
    case LogLevel::TRACE:
    case LogLevel::DEBUG:
        // TRACE 和 DEBUG 共用处理
        log_debug(msg);
        break;
    case LogLevel::INFO:
        log_info(msg);
        [[fallthrough]];  // C++17：显式声明穿透是有意的
    case LogLevel::WARN:
        log_warn(msg);
        break;
}
```

### 语法规则

```cpp
// [[fallthrough]] 必须在 case 的最后一条语句
switch (x) {
    case 1:
        a();
        [[fallthrough]];  // ✅ 在 a() 之后，声明穿透
    case 2:
        b();
        break;

    case 3:
        [[fallthrough]];  // ❌ 错误！前面没有语句
    case 4:
        break;

    case 5:
        c();
        // 没有 break 也没有 [[fallthrough]]
        // 编译器可能警告
    case 6:
        break;
}
```

### 对比 C 的 /* fall through */ 注释

```cpp
// C：注释（编译器可能不认）
switch (x) {
    case 1:
        a();
        /* fall through */
    case 2:
        break;
}

// C++17：标准属性（编译器保证识别）
switch (x) {
    case 1:
        a();
        [[fallthrough]];
    case 2:
        break;
}
```

## HFT 关联

```cpp
// 消息处理中条件编译的未使用参数
void on_tick([[maybe_unused]] const TickHeader& hdr, const TickBody& body) {
#ifdef CHECK_SEQ
    if (hdr.seq != expected_seq++) { /* ... */ }
#endif
    process(body);
}

// 状态机中的有意穿透
switch (state) {
    case State::CONNECTING:
        send_hello();
        [[fallthrough]];  // 连接后立即进入认证
    case State::AUTHENTICATING:
        send_auth();
        break;
    case State::READY:
        start_trading();
        break;
}
```

## 小结

| 属性 | 用途 | 适用位置 |
|------|------|---------|
| `[[maybe_unused]]` | 抑制"未使用"警告 | 变量、参数、函数、类型 |
| `[[fallthrough]]` | 声明 switch 穿透是有意的 | case 体内最后语句 |

---

← [本章导读](./README.md) · [下一节 →](02-nodiscard.md)
