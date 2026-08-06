# C.2 消息定义

> 附录 C · 上一节：[C.1 ATM 状态机设计](01-state-machine.md) · 下一节：[C.3 消息队列封装](03-message-queue.md)

## 这节讲什么

消息传递系统的第一步是定义消息类型。本节讲用 `std::variant` 定义类型安全的消息、`std::visit` 处理消息、以及消息设计的原则。

---

## 核心规则（代码+表格）

### 消息定义（C++17 `std::variant`）

```cpp
#include <variant>
#include <string>

// ATM 相关消息
struct card_inserted {
    std::string account;
};

struct pin_verified {};
struct pin_incorrect {};
struct cancel {};
struct withdraw {
    std::string account;
    unsigned amount;
    // 回复用的队列句柄
    messaging::sender atm_queue;
};
struct withdraw_ok {};
struct withdraw_failed {};
struct cancel_pressed {};
struct money_inserted {
    unsigned amount;
};

// 消息类型 = variant 的所有可能
using Message = std::variant<
    card_inserted,
    pin_verified,
    pin_incorrect,
    cancel,
    withdraw,
    withdraw_ok,
    withdraw_failed,
    cancel_pressed,
    money_inserted
>;
```

### `std::visit` 处理消息

```cpp
// 方式1：lambda + if-else
void handle(const Message& msg) {
    if (std::holds_alternative<card_inserted>(msg)) {
        auto& ci = std::get<card_inserted>(msg);
        // 处理 card_inserted
    } else if (std::holds_alternative<pin_verified>(msg)) {
        // 处理 pin_verified
    }
    // ...
}

// 方式2：std::visit + overloaded（更优雅）
struct handler {
    ATM& atm;
    void operator()(const card_inserted& m) { atm.on_card_inserted(m); }
    void operator()(const pin_verified& m) { atm.on_pin_verified(m); }
    void operator()(const pin_incorrect& m) { atm.on_pin_incorrect(m); }
    void operator()(const cancel& m) { atm.on_cancel(m); }
    // ...
};

void ATM::process(const Message& msg) {
    std::visit(handler{*this}, msg);  // 自动分发到正确的重载
}

// C++20 的 overloaded 技巧
template <typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::visit(overloaded{
    [&](const card_inserted& m) { /* ... */ },
    [&](const pin_verified& m) { /* ... */ },
    [&](const auto& other) { /* 默认处理 */ }
}, msg);
```

### 消息设计原则

| 原则 | 说明 |
|------|------|
| 不可变 | 消息一旦发送不应修改（值语义） |
| 自包含 | 消息包含所有必要信息（不引用共享状态） |
| 类型安全 | 用 variant 而非 void* |
| 小而专 | 一个消息一个意图，不要"万能消息" |
| 可序列化 | 未来可能需要跨进程/跨机器传递 |

### 消息中携带回复队列

```cpp
// 消息中携带"回复到哪里"
struct withdraw {
    std::string account;
    unsigned amount;
    messaging::sender reply_to;  // 回复用的队列句柄
};

// 银行线程处理完 withdraw 后，通过 reply_to 回复
void bank_process(withdraw& w) {
    if (check_balance(w.account, w.amount)) {
        w.reply_to.send(withdraw_ok{});
    } else {
        w.reply_to.send(withdraw_failed{});
    }
}
// 这样消息发送方不需要知道接收方的队列，只通过 reply_to 回复
```

---

## 新手要点（和 C 的区别）

- **C 用 `struct` + tag 字段实现消息**：
  ```c
  struct Message {
      int type;  // CARD_INSERTED / PIN_VERIFIED / ...
      char account[32];
      unsigned amount;
  };
  ```
  这种方式不安全——`type` 和字段不匹配时 UB。C++ 的 `std::variant` 是类型安全的——编译器保证访问正确的类型。
- **`std::visit` 是 C 程序员陌生的新工具**：C 程序员用 `switch (type)` 分发——容易漏 case。`std::visit` 强制处理所有类型——漏了编译报错。
- **"消息携带回复队列"是 C 程序员可能没想到的设计**：C 程序员可能用全局变量或回调函数标识回复目标——但消息传递中，回复队列作为消息的一部分，更解耦。
- **值语义 vs 指针语义**：C 程序员可能用指针传递消息（避免拷贝）——但指针指向共享状态就破坏了"不共享"原则。C++ 的 variant 值语义让消息自包含——拷贝传递，无共享。

---

## HFT 关联

- **HFT 的消息定义用 `std::variant`**：行情消息（Tick/Snapshot/Trade）、订单消息（NewOrder/Cancel/Replace）用 variant 定义——类型安全、编译器保证穷尽处理。
- **消息携带回复队列在 HFT 中的应用**：策略线程给风控线程发审核请求时，携带自己的回复队列——风控审核完直接回复到策略线程的队列，无需共享状态。
- **消息从 mempool 分配**：HFT 中如果消息较大（如完整快照），从 mempool 分配，消息中存指针——但所有权清晰（接收方负责归还 mempool）。
- **消息可序列化**：HFT 的消息可能需要记录到日志（回放）或跨进程传递——设计时考虑序列化接口。

---

## 自测题

1. `std::variant` 相比 C 的 `struct + tag` 有什么优势？
2. `std::visit` 相比 `switch-case` 有什么好处？
3. 消息设计有哪些原则？为什么消息应该"自包含"？
4. 为什么消息中要携带"回复队列"？有什么好处？
5. HFT 的消息定义为什么用值语义而非指针？

---

## 参考与延伸

- 下一节：[C.3 消息队列封装](03-message-queue.md)
- 上一节：[C.1 ATM 状态机设计](01-state-machine.md)
- 回到：[附录 C](README.md)
