# C.1 ATM 状态机设计

> 附录 C ATM 综合实战 · 上一章：[B.6 选型建议](../appendix-b-library-comparison/06-recommendations.md) · 下一节：[C.2 消息定义](02-messages.md)

## 这节讲什么

ATM（自动取款机）是全书唯一的完整案例——演示消息传递风格的并发设计。本节讲 ATM 的状态机建模、为什么状态机适合消息传递、以及状态转换的实现。

---

## 核心规则（代码+表格）

### ATM 的状态机

```
等待卡片 → 验证 PIN → 选择业务 → 处理取款 → 打印凭条 → 完成
                          ↓
                      查询余额 → 完成
                          ↓
                      取消 → 退卡
```

每个状态：
- 接收消息
- 处理（可能发送消息给其他线程）
- 转到下一状态

### 状态机的消息传递实现

```cpp
// 每个状态是一个函数，接收消息，返回下一状态
using State = std::function<void(ATM&, const Message&)>;

void waiting_for_card(ATM& atm, const Message& msg) {
    if (std::holds_alternative<card_inserted>(msg)) {
        auto& ci = std::get<card_inserted>(msg);
        atm.account = ci.account;
        atm.send_to_bank(verify_pin{ci.account, atm.get_pin()});
        atm.state = &verifying_pin;
    }
}

void verifying_pin(ATM& atm, const Message& msg) {
    if (std::holds_alternative<pin_verified>(msg)) {
        atm.display("Enter amount");
        atm.state = &waiting_for_amount;
    } else if (std::holds_alternative<pin_incorrect>(msg)) {
        atm.display("PIN incorrect");
        atm.state = &waiting_for_card;
    }
}

void waiting_for_amount(ATM& atm, const Message& msg) {
    if (std::holds_alternative<withdraw>(msg)) {
        auto& w = std::get<withdraw>(msg);
        atm.send_to_bank(withdraw_request{atm.account, w.amount});
        atm.state = &processing_withdrawal;
    }
}

// ATM 主循环
void ATM::run() {
    state = &waiting_for_card;
    while (running) {
        Message msg = incoming.wait_and_pop();
        state(*this, msg);  // 调用当前状态处理函数
    }
}
```

### 为什么状态机适合消息传递

| 特点 | 状态机 | 消息传递 |
|------|--------|----------|
| 状态明确 | 每个状态是独立函数 | 每个线程有独立状态 |
| 状态转换 = 消息发送 | 转换时发送消息 | 消息触发状态变化 |
| 无共享 | 状态机内部状态私有 | 线程间不共享可变状态 |
| 可回放 | 消息序列决定状态序列 | 消息可记录回放 |

### 状态机的线程模型

```
ATM 线程          银行线程          硬件线程
   │                 │                 │
   ├──withdraw──────→│                 │
   │                 │                 │
   │←─withdraw_ok────┤                 │
   │                 │                 │
   ├──dispense─────────────────────────→│
   │                 │                 │
   │←──card_ejected─────────────────────┤
```

每个线程有独立的邮箱（消息队列），线程间只通过消息通信。

---

## 新手要点（和 C 的区别）

- **C 程序员可能用 switch-case 实现状态机**：`switch (state) { case WAITING: ... }`——这是 C 的经典写法。C++ 用函数指针（`std::function`）让每个状态是独立函数——更模块化、更易扩展。
- **消息传递 vs 函数调用**：C 程序员可能习惯直接调用 `bank.verify_pin()`——但这是共享内存/同步调用。消息传递是发送消息到队列，由银行线程异步处理——解耦且无锁。
- **`std::variant` 是类型安全的联合体**：C 用 `union` 或 `void*` + tag 实现消息——不安全。C++17 的 `std::variant` + `std::visit` 是类型安全的消息传递工具。
- **状态机是并发友好的设计**：C 程序员可能觉得状态机"只是设计模式"——但在并发中，状态机让"线程私有状态"自然化——每个状态函数只访问本线程的私有数据，无共享。

---

## HFT 关联

- **HFT 策略引擎是状态机**：策略有"等待信号→持仓→止盈/止损→平仓"等状态——用消息传递状态机实现，每状态独立函数，消息驱动转换。
- **订单状态机**：订单有"待发送→已发送→部分成交→全部成交/取消"等状态——用状态机管理，消息驱动（交易所回报）。
- **消息传递 = 可回放**：HFT 的消息流可以记录——出问题时回放消息，精确重现状态机执行路径。这是 HFT 调试的重要手段。
- **状态机避免共享**：HFT 的策略状态机用消息传递——状态数据是线程私有的，通过消息和其他线程交互。无共享 = 无竞争。

---

## 自测题

1. ATM 为什么用状态机建模？状态机有什么优势？
2. 状态机的每个"状态"如何实现？如何表示状态转换？
3. 为什么消息传递和状态机是天然搭配？
4. C++ 的 `std::variant` 相比 C 的 `union` 有什么优势？
5. HFT 的策略引擎如何用状态机+消息传递实现？

---

## 参考与延伸

- 下一节：[C.2 消息定义](02-messages.md)
- 上一章：[B.6 选型建议](../appendix-b-library-comparison/06-recommendations.md)
- 回到：[附录 C](README.md)
