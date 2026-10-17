# 3.1 mutex + RAII 锁

> 第 3 章 共享数据 · 下一节：[3.2 死锁及避免](02-deadlock.md)

## 这节讲什么

`mutex` 是保护共享数据的基本工具。`lock_guard`/`unique_lock` 的 RAII 保证异常安全——永远不要手写 `lock()`/`unlock()`。

## 为什么要学这个（先建立直觉）

C 程序员习惯手动加锁解锁：

```c
// C：手动 lock/unlock——异常路径容易忘记
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void buggy() {
    pthread_mutex_lock(&mtx);
    do_work();  // 如果这里出错 return，忘了 unlock → 死锁
    pthread_mutex_unlock(&mtx);
}

// 改进版：用 goto cleanup
void better() {
    pthread_mutex_lock(&mtx);
    if (error) goto cleanup;
    do_work();
cleanup:
    pthread_mutex_unlock(&mtx);
}
// 仍然容易遗漏，特别是多个 return 点
```

C++ 的 RAII 锁自动解决这个问题——构造加锁，析构解锁，无论正常返回还是异常：

```cpp
// C++：RAII 锁——所有路径自动解锁
std::mutex m;

void safe() {
    std::lock_guard<std::mutex> lk(m);  // 构造即锁
    do_work();  // 抛异常？析构仍然解锁
    if (error) return;  // 提前返回？析构仍然解锁
}  // 正常结束？析构解锁
```

## 核心用法详解

### lock_guard：最简 RAII 锁

```cpp
std::mutex m;

{
    std::lock_guard<std::mutex> lk(m);  // 构造：m.lock()
    // 临界区：安全访问共享数据
    shared_data.push_back(42);
}  // 析构：m.unlock()
```

特点：
- 构造即锁，析构即解
- 不可提前解锁、不可延迟加锁、不可转移
- 零开销抽象（编译器优化后和无 RAII 版本一样快）

### unique_lock：灵活 RAII 锁

```cpp
std::mutex m;

// 1. 延迟加锁
std::unique_lock<std::mutex> lk(m, std::defer_lock);  // 不加锁
// ... 做一些不需要锁的事 ...
lk.lock();  // 现在才加锁

// 2. 提前解锁
lk.unlock();  // 释放锁
// ... 做一些不需要锁的事 ...
lk.lock();  // 重新加锁

// 3. 配合 condition_variable（必须用 unique_lock）
std::condition_variable cv;
cv.wait(lk, [&]{ return ready; });  // wait 内部会 unlock+wait+relock

// 4. 转移所有权
std::unique_lock<std::mutex> lk2 = std::move(lk);
```

### 何时用哪个

| 特性 | lock_guard | unique_lock |
|------|-----------|-------------|
| 性能 | 最优（零开销） | 略有开销（维护锁状态标志） |
| 灵活性 | 低 | 高 |
| 延迟加锁 | 不支持 | `defer_lock` |
| 提前解锁 | 不支持 | `unlock()` |
| 配合 cv | 不支持 | 必须 |
| 转移所有权 | 不支持 | `std::move` |
| 默认选择 | ✅ 优先 | 需要灵活性时 |

## 常见错误（新手踩坑）

### 错误 1：手写 lock/unlock

```cpp
// 错误：手动管理锁
m.lock();
if (error) {
    // 忘了 unlock → 死锁
    return;
}
do_work();
m.unlock();
```

**修复**：永远用 RAII 锁——`lock_guard` 或 `unique_lock`。

### 错误 2：锁粒度太细

```cpp
// 错误：每个操作各自加锁，中间有竞争窗口
class Stack {
    std::vector<int> data;
    std::mutex m;
public:
    bool empty() {
        std::lock_guard<std::mutex> lk(m);
        return data.empty();
    }
    int top() {
        std::lock_guard<std::mutex> lk(m);
        return data.back();
    }
    void pop() {
        std::lock_guard<std::mutex> lk(m);
        data.pop_back();
    }
};

// 调用方：empty() 和 top() 之间可能被其他线程 pop！
if (!stack.empty()) {
    auto v = stack.top();  // 可能此时已被其他线程 pop → top 空栈 UB
    stack.pop();           // pop 空栈 UB
}
```

**修复**：提供原子操作接口（持锁覆盖整个逻辑操作），见 3.3 节。

### 错误 3：锁粒度太粗

```cpp
// 错误：整个循环都在锁内，其他线程完全被阻塞
std::lock_guard<std::mutex> lk(m);
for (int i = 0; i < 1000000; i++) {
    data[i] = compute(i);  // compute 很慢，但不需要锁
}

// 修复：只锁数据访问部分
for (int i = 0; i < 1000000; i++) {
    auto val = compute(i);  // 锁外计算
    std::lock_guard<std::mutex> lk(m);
    data[i] = val;  // 锁内写入
}
```

## 和 C 的区别

| 特性 | C (pthread) | C++ (std::mutex) |
|------|-------------|------------------|
| 锁类型 | `pthread_mutex_t` | `std::mutex` |
| 加锁 | `pthread_mutex_lock(&m)` | `m.lock()` 或 RAII |
| RAII 锁 | 无（手动或自封装） | `lock_guard`/`unique_lock` |
| 异常安全 | 无 | RAII 保证 |
| 递归锁 | `PTHREAD_MUTEX_RECURSIVE` | `std::recursive_mutex` |
| 读写锁 | `pthread_rwlock_t` | `std::shared_mutex`（C++17） |

## HFT 关联

- **热路径避锁**：mutex 有上下文切换（~1-10μs）+ 调度抖动风险，HFT 热路径用无锁结构（`atomic`/SPSC 队列）替代。
- **锁竞争是延迟杀手**：多个线程争抢同一 mutex，等待时间不确定——HFT 需要可预测的延迟，锁竞争引入的抖动不可接受。
- **自旋锁在特定场景更优**：等待时间 < 上下文切换开销时，自旋（`spinlock`）比 mutex 更快。HFT 短临界区用自旋锁，但要注意 `pause` 指令和避免优先级反转。

## 代码自测

### Q1: 下列代码有什么问题？

```cpp
std::mutex m;
void worker() {
    m.lock();
    if (data_ready) {
        process();
        m.unlock();
    }
    // else 分支没有 unlock！
}
```

<details>
<summary>答案与复习指引</summary>

**else 分支忘记 unlock → 死锁**。如果 `data_ready` 为 false，锁永远不释放，其他线程全部阻塞。

修复：用 RAII 锁，或确保所有路径都 unlock：
```cpp
std::lock_guard<std::mutex> lk(m);
if (data_ready) process();
```

复习：永远不要手写 `lock()`/`unlock()`——RAII 锁保证所有路径（含异常/提前返回）都解锁。
</details>

### Q2: 下列两个版本哪个更好？

```cpp
// A:
std::lock_guard<std::mutex> lk(m);
for (auto& item : items) {
    item = transform(item);  // transform 很慢
}

// B:
for (auto& item : items) {
    auto tmp = transform(item);  // 锁外计算
    std::lock_guard<std::mutex> lk(m);
    item = tmp;  // 锁内写入
}
```

<details>
<summary>答案与复习指引</summary>

**B 更好**（如果 `transform` 不需要锁保护）。A 在整个循环期间持有锁，其他线程被完全阻塞。B 只在写入时持锁，计算期间其他线程可以并行工作。

但 B 创建了多个 `lock_guard`（每次循环一个），有微小的构造/析构开销。如果 `transform` 很快，A 可能更好。

复习：锁粒度原则——只锁必须锁的部分，锁内不做不需要锁的工作（IO、计算等）。
</details>

### Q3: 下列代码能编译吗？

```cpp
std::condition_variable cv;
std::mutex m;
std::lock_guard<std::mutex> lk(m);
cv.wait(lk, [&]{ return ready; });  // 用 lock_guard 等待
```

<details>
<summary>答案与复习指引</summary>

**编译错误**。`cv.wait()` 要求 `std::unique_lock`（因为 wait 内部需要 unlock/relock，`lock_guard` 不支持提前解锁）。

修复：把 `lock_guard` 改成 `unique_lock`：
```cpp
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return ready; });
```

复习：`condition_variable::wait` 必须用 `unique_lock`——它需要在等待时 unlock、唤醒后 relock。
</details>

### Q4: 为什么 HFT 热路径用无锁结构而不是 mutex？

<details>
<summary>答案与复习指引</summary>

三个原因：
1. **延迟不确定性**：mutex 等待时间取决于竞争程度——可能 10ns 也可能 10μs，HFT 无法接受这种波动。
2. **上下文切换**：mutex 竞争时线程被 park（让出 CPU），上下文切换 ~1-10μs + cache 丢失。
3. **优先级反转**：低优先级线程持锁，高优先级线程等待——HFT 的关键线程可能被低优先级线程阻塞。

无锁结构（atomic/SPSC 队列）延迟固定（~10-100ns），无上下文切换，适合热路径。

复习：mutex 适合非热路径（初始化、配置更新等）。HFT 热路径用无锁结构。
</details>

---

## 参考与延伸

- 下一节：[3.2 死锁及避免](02-deadlock.md)
- 回到：[第 3 章](README.md)
