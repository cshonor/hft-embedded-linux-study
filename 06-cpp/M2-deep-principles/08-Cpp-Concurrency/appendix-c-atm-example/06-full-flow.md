# C.6 完整流程

> 附录 C · 上一节：[C.5 为什么用消息传递](05-why-msg-passing.md) · 下一章：[附录 D.1 thread 线程管理](../appendix-d-thread-library-ref/01-thread.md)

## 这节讲什么

把 ATM 案例的完整流程串起来——从插卡到取款完成，展示消息在三个线程（ATM、银行、硬件）之间的完整流动。

---

## 核心规则（代码+表格）

### 完整消息流

```
用户插入卡片
    ↓
[硬件线程] → ATM: card_inserted{account="123456"}
    ↓
[ATM] 状态: waiting_for_card → waiting_for_pin
[ATM] → 硬件: enter_pin{}
    ↓
用户输入 PIN
    ↓
[硬件线程] → ATM: pin_entered{pin=1234}
    ↓
[ATM] 状态: waiting_for_pin → verifying_pin
[ATM] → 银行: verify_pin{account="123456", pin=1234, reply_to=atm_sender}
    ↓
[银行] 验证 PIN...
    ↓
[银行] → ATM: pin_verified
    ↓
[ATM] 状态: verifying_pin → waiting_for_action
[ATM] → 硬件: display{"Select action"}
    ↓
用户选择取款并输入金额
    ↓
[硬件线程] → ATM: withdraw{account="123456", amount=500}
    ↓
[ATM] 状态: waiting_for_action → processing_withdrawal
[ATM] → 银行: withdraw_request{account="123456", amount=500}
    ↓
[银行] 检查余额...
    ↓
[银行] → ATM: withdraw_ok
    ↓
[ATM] → 硬件: issue_money{500}
[ATM] → 银行: withdraw_processed{account="123456", amount=500}
[ATM] 状态: processing_withdrawal → done
    ↓
[ATM] → 硬件: eject_card{}
[ATM] 状态: done → waiting_for_card
    ↓
用户取走现金和卡片
```

### 三个线程的职责

| 线程 | 职责 | 拥有的状态 |
|------|------|-----------|
| ATM | 状态机逻辑 | 当前状态、账号、金额 |
| 银行 | 账户验证、余额管理 | 账户数据库 |
| 硬件 | 物理设备交互 | 设备状态 |

### 消息流的时间线

```
T=0.000  硬件 → ATM: card_inserted
T=0.001  ATM → 硬件: enter_pin
T=0.500  硬件 → ATM: pin_entered
T=0.501  ATM → 银行: verify_pin
T=0.510  银行 → ATM: pin_verified
T=0.511  ATM → 硬件: display("Select action")
T=1.000  硬件 → ATM: withdraw
T=1.001  ATM → 银行: withdraw_request
T=1.020  银行 → ATM: withdraw_ok
T=1.021  ATM → 硬件: issue_money
T=1.022  ATM → 银行: withdraw_processed
T=1.023  ATM → 硬件: eject_card
```

### 异常流程：余额不足

```
...
T=1.001  ATM → 银行: withdraw_request{amount=500}
T=1.020  银行 → ATM: withdraw_failed  ← 余额不足
T=1.021  ATM → 硬件: display("Insufficient funds")
T=1.022  ATM → 硬件: eject_card
T=1.023  ATM 状态: → waiting_for_card
```

### 异常流程：用户取消

```
任何状态下，用户按取消键：
[硬件] → ATM: cancel_pressed
[ATM] → 硬件: eject_card
[ATM] 状态: → done → waiting_for_card
```

### 关键设计点

| 设计 | 说明 |
|------|------|
| 每线程私有状态 | ATM/银行/硬件各自管理自己的数据，不共享 |
| 消息携带回复队列 | `verify_pin` 携带 `reply_to`，银行直接回复 |
| 状态机驱动 | ATM 的行为完全由当前状态 + 收到的消息决定 |
| 无锁 | 线程间通过队列通信，无 mutex |
| 可回放 | 消息流记录后可精确重现 |

---

## 新手要点（和 C 的区别）

- **C 程序员可能用全局状态机 + 多线程操作**：C 的 ATM 实现可能是"全局状态变量 + 多线程读写"——竞争风险高。C++ 消息传递让每线程私有状态——无竞争。
- **"消息流时间线"是 C 程序员可能没见过的调试工具**：C 程序员可能用 gdb 单步——但并发中不可行。消息流时间线是事后分析的工具——记录所有消息，画出时间线。
- **异常流程的处理**：C 程序员可能用错误码 + 全局 error flag——消息传递用不同的消息类型（`withdraw_failed` vs `withdraw_ok`）——更清晰。
- **"消息携带回复队列"是关键设计**：C 程序员可能用全局变量标识"回复给谁"——消息传递把回复队列作为消息的一部分——更解耦、更安全。

---

## HFT 关联

- **HFT 的完整流程和 ATM 完全一致**：行情采集（硬件线程）→ 解析（ATM 线程）→ 策略（银行线程）→ 下单（硬件线程）——消息在流水线中流动，每线程私有状态。
- **消息流时间线 = HFT 的审计日志**：HFT 系统记录所有消息（tick、order、fill）——出问题时画出时间线，精确重现。这是金融合规的要求。
- **异常流程处理**：HFT 系统的异常（如订单被拒、行情中断）用不同的消息类型表达——和 ATM 的 `withdraw_failed` 一样，类型安全且可回放。
- **取消流程**：HFT 的"一键撤单"类似 ATM 的 `cancel_pressed`——发取消消息到各线程，各线程在安全点退出——协作式中断。

---

## 自测题

1. ATM 取款的完整消息流经过哪些线程？画出一个简图。
2. 三个线程（ATM/银行/硬件）各自拥有什么状态？为什么不共享？
3. 消息中为什么要携带"回复队列"（`reply_to`）？
4. 如果银行余额不足，消息流会怎样变化？
5. HFT 的完整流程和 ATM 案例有什么相似之处？

---

## 参考与延伸

- 下一章：[附录 D.1 thread 线程管理](../appendix-d-thread-library-ref/01-thread.md)
- 上一节：[C.5 为什么用消息传递](05-why-msg-passing.md)
- 回到：[附录 C](README.md)
