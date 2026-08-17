# C.3 消息队列封装

> 附录 C · 上一节：[C.2 消息定义](02-messages.md) · 下一节：[C.4 ATM 状态机实现](04-atm-implementation.md)

## 这节讲什么

消息传递的基础设施是消息队列。本节讲一个简单的阻塞消息队列封装——`sender`（发送端句柄）和 `receiver`（接收端），用 `std::queue` + `mutex` + `condition_variable` 实现。

---

## 核心规则（代码+表格）

### 消息队列实现

```cpp
namespace messaging {

// 前向声明
class queue;

// 发送端：轻量句柄，可以拷贝传递
class sender {
    queue* q;
public:
    sender() : q(nullptr) {}
    explicit sender(queue* q) : q(q) {}
    
    template <typename Message>
    void send(Message&& msg) {
        if (q) q->push(std::forward<Message>(msg));
    }
};

// 接收端：独占，不可拷贝
class receiver {
    std::unique_ptr<queue> q;
public:
    receiver() : q(std::make_unique<queue>()) {}
    
    sender get_sender() { return sender(q.get()); }
    
    // 等待并取出消息
    template <typename F>
    void wait_and_dispatch(F&& handler) {
        q->wait_and_dispatch(std::forward<F>(handler));
    }
    
    // 非阻塞尝试
    template <typename F>
    bool try_dispatch(F&& handler) {
        return q->try_dispatch(std::forward<F>(handler));
    }
};

// 内部队列实现
class queue {
    std::mutex m;
    std::condition_variable cv;
    std::queue<Message> messages;
public:
    template <typename Message>
    void push(Message&& msg) {
        {
            std::lock_guard<std::mutex> lk(m);
            messages.push(std::forward<Message>(msg));
        }
        cv.notify_one();
    }
    
    Message wait_and_pop() {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [this]{ return !messages.empty(); });
        auto msg = std::move(messages.front());
        messages.pop();
        return msg;
    }
    
    bool try_pop(Message& msg) {
        std::lock_guard<std::mutex> lk(m);
        if (messages.empty()) return false;
        msg = std::move(messages.front());
        messages.pop();
        return true;
    }
};

} // namespace messaging
```

### sender / receiver 的设计

| 类型 | 可拷贝？ | 所有权 | 用途 |
|------|---------|--------|------|
| `sender` | 是 | 共享队列指针 | 传递给其他线程，用于发消息 |
| `receiver` | 否（可 move） | 独占队列 | 本线程接收消息 |

```cpp
// ATM 线程创建 receiver
receiver atm_recv;
sender atm_sender = atm_recv.get_sender();

// 把 atm_sender 传给其他线程（如硬件线程）
hardware_thread.set_atm_sender(atm_sender);

// 其他线程通过 atm_sender 给 ATM 发消息
atm_sender.send(card_inserted{"123456"});

// ATM 线程从 receiver 等待消息
Message msg = atm_recv.wait_and_pop();
```

### 阻塞 vs 非阻塞

```cpp
// 阻塞：等到有消息
Message msg = recv.wait_and_pop();  // 无消息时阻塞

// 非阻塞：立即返回
Message msg;
if (recv.try_pop(msg)) {
    // 有消息
} else {
    // 无消息
}

// 超时等待
Message msg;
{
    std::unique_lock<std::mutex> lk(m);
    if (cv.wait_for(lk, 100ms, [&]{ return !messages.empty(); })) {
        msg = std::move(messages.front());
        messages.pop();
    }
}
```

---

## 新手要点（和 C 的区别）

- **C 程序员通常手写队列 + pthread_mutex + pthread_cond**：功能相同，但 C++ 的 RAII（`lock_guard`/`unique_lock`）让锁管理更安全——不会忘记 unlock。C 程序员转型 C++ 时要改掉手动 lock/unlock 的习惯。
- **`sender`/`receiver` 分离是设计亮点**：C 程序员可能用一个 `Queue*` 给所有线程——但这样接收端也可能被其他线程误操作。C++ 的 sender（可拷贝、只发）和 receiver（独占、只收）分离，让权限清晰。
- **`unique_ptr` 管理队列生命周期**：C 程序员可能用 `malloc`/`free`——C++ 的 `unique_ptr` 让 receiver 析构时自动销毁队列。RAII 自动管理。
- **`std::variant` 让消息类型安全**：C 程序员的队列通常存 `void*`——类型不安全。C++ 的 `queue<Message>`（Message 是 variant）让队列类型安全。

---

## HFT 关联

- **HFT 热路径用无锁 SPSC 队列而非这里的阻塞队列**：这个阻塞队列有 mutex——HFT 热路径不能用。但管理面（如 ATM 交互、配置更新）可以用阻塞队列——简洁、安全。
- **`sender`/`receiver` 分离在 HFT 中的应用**：HFT 的流水线中，上游线程把 `sender` 传给下游——下游通过 sender 发消息，无需知道上游的完整队列接口。
- **阻塞队列用于管理面**：HFT 的监控线程、日志线程可以用阻塞队列——等待消息时睡眠，不占 CPU。热路径用无锁队列（轮询）。
- **超时等待**：HFT 的某些场景（如等订单回报）用超时等待——超时后触发重试或告警。

---

## 自测题

1. `sender` 和 `receiver` 为什么要分离？各自的可拷贝性如何？
2. 阻塞 `wait_and_pop` 和非阻塞 `try_pop` 有什么区别？
3. `condition_variable::wait_for` 的超时等待如何使用？
4. 为什么 HFT 热路径不用阻塞队列？管理面可以用吗？
5. `unique_ptr` 在 receiver 中的作用是什么？

---

## 参考与延伸

- 下一节：[C.4 ATM 状态机实现](04-atm-implementation.md)
- 上一节：[C.2 消息定义](02-messages.md)
- 回到：[附录 C](README.md)
