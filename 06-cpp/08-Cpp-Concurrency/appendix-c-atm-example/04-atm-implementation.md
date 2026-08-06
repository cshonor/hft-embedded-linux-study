# C.4 ATM 状态机实现

> 附录 C · 上一节：[C.3 消息队列封装](03-message-queue.md) · 下一节：[C.5 为什么用消息传递](05-why-msg-passing.md)

## 这节讲什么

把前 3 节的状态机、消息、队列组合起来——实现完整的 ATM 状态机。本节展示 ATM 类的完整代码骨架，每个状态如何处理消息并转换。

---

## 核心规则（代码+表格）

### ATM 类骨架

```cpp
class ATM {
    messaging::receiver incoming;
    messaging::sender bank;
    messaging::sender hardware;
    std::string account;
    unsigned withdrawal_amount = 0;

    // 状态处理函数
    void waiting_for_card();
    void waiting_for_pin();
    void verifying_pin();
    void waiting_for_amount();
    void processing_withdrawal();
    void done();

    using StateFn = void (ATM::*)();  // 成员函数指针
    StateFn state = &ATM::waiting_for_card;

public:
    ATM(messaging::sender bank_, messaging::sender hardware_)
        : bank(bank_), hardware(hardware_) {}

    messaging::sender get_sender() { return incoming.get_sender(); }

    void run() {
        (this->*state)();  // 调用当前状态函数
    }

    void done_processing() {
        state = &ATM::waiting_for_card;
        hardware.send(display_balance{0});
    }
};
```

### 状态函数实现

```cpp
void ATM::waiting_for_card() {
    hardware.send(display{"Insert card"});
    incoming.wait()
        .handle<card_inserted>([&](const card_inserted& msg) {
            account = msg.account;
            hardware.send(enter_pin{});
            state = &ATM::waiting_for_pin;
        })
        .handle<cancel_pressed>([&](const cancel_pressed&) {
            state = &ATM::done;
        });
}

void ATM::waiting_for_pin() {
    incoming.wait()
        .handle<pin_entered>([&](const pin_entered& msg) {
            bank.send(verify_pin{account, msg.pin, incoming.get_sender()});
            state = &ATM::verifying_pin;
        })
        .handle<cancel_pressed>([&](const cancel_pressed&) {
            state = &ATM::done;
        });
}

void ATM::verifying_pin() {
    incoming.wait()
        .handle<pin_verified>([&](const pin_verified&) {
            hardware.send(display{"Select action"});
            state = &ATM::waiting_for_action;
        })
        .handle<pin_incorrect>([&](const pin_incorrect&) {
            hardware.send(display("PIN incorrect"));
            state = &ATM::waiting_for_card;
        });
}

void ATM::processing_withdrawal() {
    incoming.wait()
        .handle<withdraw_ok>([&](const withdraw_ok&) {
            hardware.send(issue_money{withdrawal_amount});
            bank.send(withdraw_processed{account, withdrawal_amount});
            state = &ATM::done;
        })
        .handle<withdraw_failed>([&](const withdraw_failed&) {
            hardware.send(display("Insufficient funds"));
            state = &ATM::done;
        });
}
```

### 状态转换图

```
waiting_for_card
    ↓ card_inserted
waiting_for_pin
    ↓ pin_entered
verifying_pin
    ↓ pin_verified          ↓ pin_incorrect
waiting_for_action          waiting_for_card
    ↓ withdraw
processing_withdrawal
    ↓ withdraw_ok           ↓ withdraw_failed
done                        done
    ↓ (reset)
waiting_for_card
```

### 消息处理的 fluent 接口

```cpp
// 书中用 fluent 接口处理消息
incoming.wait()
    .handle<TypeA>([&](const TypeA& msg) { /* ... */ })
    .handle<TypeB>([&](const TypeB& msg) { /* ... */ })
    .handle<TypeC>([&](const TypeC& msg) { /* ... */ });
// wait() 阻塞等待消息，handle<T> 注册类型 T 的处理器
// 收到消息后，按类型分发到对应的 handler
// 如果消息类型不匹配任何 handler → 继续等待下一条
```

### 完整的线程模型

```cpp
int main() {
    // 银行线程
    messaging::receiver bank_recv;
    messaging::sender bank_sender = bank_recv.get_sender();
    
    // 硬件线程
    messaging::receiver hw_recv;
    messaging::sender hw_sender = hw_recv.get_sender();
    
    // ATM（自己的 receiver）
    ATM atm(bank_sender, hw_sender);
    messaging::sender atm_sender = atm.get_sender();
    
    // 启动各线程
    std::thread bank_thread(bank_machine{bank_recv});
    std::thread hw_thread(hardware_machine{hw_recv});
    std::thread atm_thread([&]{ atm.run(); });
    
    // 模拟插卡
    atm_sender.send(card_inserted{"1234567890"});
    
    atm_thread.join();
    bank_thread.join();
    hw_thread.join();
}
```

---

## 新手要点（和 C 的区别）

- **C 程序员会用 switch-case + 函数指针**：
  ```c
  switch (state) {
      case WAITING_FOR_CARD: handle_waiting_for_card(); break;
      case VERIFYING_PIN: handle_verifying_pin(); break;
  }
  ```
  C++ 用成员函数指针（`StateFn`）让状态 = 函数——更模块化、更类型安全。
- **fluent 接口是 C++ 风格**：C 程序员可能不熟悉 `.handle<A>(...).handle<B>(...)` 的链式调用——这是 C++ 的 fluent API 设计。C 通常用回调数组或 switch-case。
- **消息类型分发**：C 程序员用 `if (msg.type == CARD_INSERTED)`——容易漏 case。C++ 的 `.handle<T>()` 让编译器检查类型——更安全。
- **"状态 = 函数指针"让状态机可扩展**：C 程序员的 switch-case 加状态要改 switch——C++ 只需加一个成员函数 + 修改状态转换点。

---

## HFT 关联

- **HFT 策略引擎可以借鉴 ATM 状态机**：策略的"等待信号→建仓→持仓→平仓"用同样的状态机模式——每状态一个函数，消息驱动转换。
- **消息处理器的 fluent 接口**：HFT 策略可以类似地 `.handle<Tick>(...).handle<Order>(...).handle<Timeout>(...)`——类型安全且可读。
- **每线程独立 receiver**：HFT 流水线的每阶段线程有自己的 receiver——上游通过 sender 发消息，下游从 receiver 等待。和 ATM 完全一致。
- **状态机的可回放性**：ATM 的消息流可记录——回放消息，状态机重现执行路径。HFT 的策略状态机同样可回放——出问题时精确重现。

---

## 自测题

1. ATM 状态机如何用成员函数指针表示状态？
2. 状态转换是如何实现的？（提示：修改 `state` 成员）
3. fluent 接口 `.handle<T>(...)` 相比 switch-case 有什么优势？
4. ATM、银行、硬件三个线程如何通过 sender/receiver 通信？
5. HFT 策略引擎如何借鉴 ATM 状态机模式？

---

## 参考与延伸

- 下一节：[C.5 为什么用消息传递](05-why-msg-passing.md)
- 上一节：[C.3 消息队列封装](03-message-queue.md)
- 回到：[附录 C](README.md)
