# 附录 C ATM 综合实战

**A Message-Passing Example: The ATM**

## 本附录讲什么

用一个完整的 ATM（自动取款机）模拟系统，演示**消息传递**风格的并发设计——线程间不共享数据，靠消息队列通信。这是 actor 模型在 C++ 中的实践，也是全书的综合应用。

## 要点

### 设计思路：状态机 + 消息传递

ATM 逻辑天然是状态机：

```
等待卡片 → 验证 PIN → 选择业务 → 处理取款 → 打印凭条 → 完成
                          ↓
                      查询余额
```

每个状态接收消息、处理、发送消息、转到下一状态。线程间**不共享可变状态**，只通过消息队列通信。

### 消息定义

```cpp
struct withdraw {
    std::string account;
    unsigned amount;
    messaging::sender atm_queue;   // 回复用的队列
};
struct withdraw_ok {};
struct withdraw_failed {};
struct cancel {};
struct card_inserted {
    std::string account;
};
struct pin_verified {};
struct pin_incorrect {};
// ...
```

消息是**纯数据结构**（POD），无逻辑——逻辑在状态机的处理函数里。

### 消息队列（SPSC 封装）

```cpp
namespace messaging {
    class queue {
        std::mutex m;
        std::condition_variable cv;
        std::queue<std::shared_ptr<message_base>> q;   // 类型擦除
    public:
        template <typename Msg>
        void send(const Msg& msg) {
            std::lock_guard<std::mutex> lk(m);
            q.push(std::make_shared<wrapped_message<Msg>>(msg));
            cv.notify_one();
        }
        // wait 接收消息并 dispatch 到 handler
    };
}
```

每个"actor"（ATM、银行、用户界面）有自己的消息队列，互相通过 `sender` 发消息。

### ATM 状态机

```cpp
class atm {
    messaging::receiver incoming;
    messaging::sender bank, interface_hardware;
    void (atm::*state)();   // 当前状态函数指针

    void wait_for_card() {
        incoming.wait()
            .handle<card_inserted>([&](const card_inserted& msg) {
                bank.send(verify_pin{msg.account, ...});
                state = &atm::verifying_pin;
            });
    }
    void verifying_pin() {
        incoming.wait()
            .handle<pin_verified>([&](const pin_verified&) {
                interface_hardware.send(display_balance{...});
                state = &atm::wait_for_action;
            })
            .handle<pin_incorrect>([&](const pin_incorrect&) {
                interface_hardware.send(display_pin_incorrect{});
                state = &atm::done_processing;
            });
    }
    // ...
public:
    void run() {
        state = &atm::wait_for_card;
        while (state != &atm::done_processing)
            (this->*state)();   // 调用当前状态函数
    }
};
```

**要点**：
- 状态用**成员函数指针**表示，`state = &atm::next_state` 切换状态。
- 每个状态函数 `wait()` 阻塞等消息，用 `.handle<T>()` 链式注册不同消息类型的处理。
- 处理完一条消息后设 `state` 指向下一状态，循环回到 `run` 再调用。

### 为什么用消息传递

| 维度 | 共享数据 + 锁 | 消息传递 |
|------|---------------|----------|
| 共享状态 | 有（需同步） | 无（每 actor 私有） |
| 死锁 | 可能（多锁交叉） | 不可能（无锁等待） |
| 扩展性 | 受锁竞争限制 | 线性好（无共享） |
| 推理难度 | 高（交错复杂） | 低（单线程状态机） |
| 性能 | 临界区短时高 | 消息拷贝有开销 |

### 完整流程

```
1. 用户插卡 → interface 发 card_inserted 给 atm
2. atm 发 verify_pin 给 bank，转 verifying_pin 状态
3. bank 验证后回 pin_verified 或 pin_incorrect
4. atm 转到 wait_for_action，等用户选业务
5. 用户选取款 → interface 发 withdraw 给 atm
6. atm 发 withdraw_request 给 bank
7. bank 回 withdraw_ok 或 withdraw_failed
8. atm 发 issue_money 给 interface，转 done_processing
```

每一步都是**单向消息**，没有"调用-返回"的同步等待——完全异步、无阻塞。

## HFT 关联

- **actor 模型与 HFT 流水线**：HFT 的"网卡线程→解析→策略→下单"流水线就是 actor 模型——每线程一个 actor，SPSC 队列就是消息队列，无共享。
- **消息用 POD 结构**：HFT 消息（行情快照、订单请求）用 POD 结构，可 memcpy、cache 友好、无虚函数开销。
- **状态机做策略状态**：策略的"初始化→就绪→交易中→暂停"用状态机管理，比一堆 `if` 清晰。
- **异步消息无阻塞**：策略线程发订单消息给下单线程后不等待，继续处理下一条行情——HFT 流水线不能阻塞。
- **消息队列用 SPSC 环形缓冲**：ATM 示例用 mutex+cv 的通用队列，HFT 换成 SPSC 无锁环形队列（零竞争、零分配）。
- **错误隔离**：一个 actor 崩溃不影响其他 actor——策略 actor 异常，风控 actor 仍能工作。HFT 风控逻辑独立线程，策略崩溃不能挡住风控。

## 自测题

1. ATM 示例为什么用消息传递而非共享数据？消息传递的四个优势是什么？
2. ATM 的状态机如何用成员函数指针实现？状态切换的机制是什么？
3. 消息为什么要用 POD 结构？类型擦除是如何实现的？
4. HFT 流水线和 ATM 的 actor 模型有什么对应关系？
5. 为什么 HFT 把通用消息队列换成 SPSC 环形缓冲？actor 模型的无阻塞特性如何保证流水线不阻塞？
