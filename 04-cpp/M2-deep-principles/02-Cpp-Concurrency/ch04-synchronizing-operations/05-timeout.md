# 4.5 超时等待

> 第 4 章 · 上一节：[4.4 future 的局限](04-future-limits.md) · 下一节：[4.6 C++20 新同步原语](06-cpp20-primitives.md)

## 这节讲什么

`wait_for`/`wait_until` 让等待有超时——避免无限阻塞。condition_variable 和 future 都支持超时。

## 为什么要学这个（先建立直觉）

C 程序员用 `pthread_cond_timedwait` 做超时等待——需要手动计算绝对时间：

```c
// C：pthread 超时等待——手动算绝对时间
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

void wait_with_timeout() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;  // 1 秒后超时

    pthread_mutex_lock(&mtx);
    int rc = pthread_cond_timedwait(&cv, &mtx, &ts);
    if (rc == ETIMEDOUT) {
        printf("timeout\n");
    } else {
        printf("signaled\n");
    }
    pthread_mutex_unlock(&mtx);
}
```

C++ 用 `std::chrono` 简化了超时计算：

```cpp
// C++：超时等待——chrono 简化
std::mutex m;
std::condition_variable cv;
bool ready = false;

std::unique_lock<std::mutex> lk(m);
if (cv.wait_for(lk, std::chrono::seconds(1), [&]{ return ready; })) {
    // 条件满足（1 秒内被通知）
} else {
    // 超时（1 秒后仍未满足）
}
```

## 核心用法详解

### condition_variable 超时

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

// wait_for：相对超时（从现在开始等多久）
std::unique_lock<std::mutex> lk(m);
bool success = cv.wait_for(lk, std::chrono::milliseconds(500),
                            [&]{ return ready; });
// success == true:  条件满足
// success == false: 超时

// wait_until：绝对超时（等到某个时间点）
auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
bool success2 = cv.wait_until(lk, deadline, [&]{ return ready; });
```

### future 超时

```cpp
std::future<int> f = std::async(std::launch::async, long_task);

// wait_for：检查是否在超时内完成
auto status = f.wait_for(std::chrono::seconds(2));
if (status == std::future_status::ready) {
    int v = f.get();  // 不阻塞，已完成
} else if (status == std::future_status::timeout) {
    // 超时，还没完成
    // 注意：不能 "取消" future——它还在后台跑
} else if (status == std::future_status::deferred) {
    // 任务还没开始（async 的 deferred 策略）
    int v = f.get();  // 此时才同步执行
}

// wait_until：绝对超时
auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
auto status2 = f.wait_until(deadline);
```

### wait_for vs wait_until

| 函数 | 参数类型 | 语义 | 适用场景 |
|------|----------|------|----------|
| `wait_for` | duration | "从现在开始等 X 时间" | 相对超时 |
| `wait_until` | time_point | "等到 X 时刻" | 绝对超时（多次等待不累计误差） |

```cpp
// wait_for 的误差问题
for (int i = 0; i < 10; i++) {
    cv.wait_for(lk, std::chrono::milliseconds(100), pred);
    // 每次 wait_for 从"当前时间"算起
    // 如果 pred 检查 + 其他开销花了 5ms
    // 10 次循环总时间 ≈ 1050ms 而非 1000ms
}

// wait_until 无累计误差
auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
while (std::chrono::steady_clock::now() < deadline) {
    cv.wait_until(lk, deadline, pred);
    // 每次都等到同一个 deadline——无累计误差
}
```

## 常见错误（新手踩坑）

### 错误 1：wait_for 不带谓词

```cpp
// 错误：无谓词——虚假唤醒时无法区分"被通知"和"超时"
cv.wait_for(lk, std::chrono::seconds(1));  // 无谓词
// 返回 false 可能是超时也可能是虚假唤醒——无法区分

// 正确：带谓词
if (cv.wait_for(lk, std::chrono::seconds(1), [&]{ return ready; }))
    // 一定是条件满足
else
    // 一定是超时
```

### 错误 2：用 system_clock 做超时

```cpp
// 错误：system_clock 可能被 NTP 调整
cv.wait_until(lk, std::chrono::system_clock::now() + std::chrono::seconds(1));
// 如果系统时间被向前调 → 超时提前触发
// 如果系统时间被向后调 → 超时延后（可能很久）

// 正确：用 steady_clock（单调递增）
cv.wait_until(lk, std::chrono::steady_clock::now() + std::chrono::seconds(1));
```

### 错误 3：超时后以为任务被取消

```cpp
// 错误：以为超时后 future 被取消
auto f = std::async(std::launch::async, long_task);
if (f.wait_for(std::chrono::seconds(1)) == std::future_status::timeout) {
    // future 没被取消！long_task 还在后台跑
    // 如果不调 get()，析构时仍会阻塞等待完成
}
```

**注意**：C++ 标准没有"取消 future"的机制。超时只是让你知道"还没完成"，不停止任务。

## 和 C 的区别

| 特性 | C (pthread) | C++ (std) |
|------|-------------|-----------|
| 超时等待 | `pthread_cond_timedwait` | `cv.wait_for`/`wait_until` |
| 时间计算 | 手动 timespec | `std::chrono` |
| 时钟类型 | CLOCK_REALTIME | steady_clock/system_clock |
| 谓词形式 | 手动 while 循环 | 内置谓词参数 |
| future 超时 | 无 | `f.wait_for`/`wait_until` |

## HFT 关联

- **超时防死等**：HFT 守护进程等待交易所响应用超时，避免网络故障时无限阻塞。
- **steady_clock 是必须的**：HFT 的超时计算必须用 `steady_clock`——`system_clock` 被 NTP 调整会导致超时不可预测。
- **超时不是取消**：HFT 超时后需要自行实现取消机制（如设置 stop flag + 线程检查），标准库不提供 future 取消。

## 代码自测

### Q1: 下列代码可能有什么问题？

```cpp
auto f = std::async(std::launch::async, [] {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 42;
});

if (f.wait_for(std::chrono::seconds(1)) == std::future_status::timeout) {
    std::cout << "timeout, giving up";
    // 不调 get()
}
// f 析构
```

<details>
<summary>答案与复习指引</summary>

**析构阻塞 9 秒**（10-1=9）。超时后不调 `get()`，但 future 析构时仍隐式 join——等待剩余 9 秒。

C++ 标准没有取消 future 的机制——超时只让你知道"还没完成"，不停止任务。

修复：如果要"真正放弃"，用 `std::thread + detach` + 自定义取消机制。

复习：future 超时 ≠ 取消。析构仍等待完成。
</details>

### Q2: 下列代码用哪个时钟更好？

```cpp
// A:
cv.wait_until(lk, std::chrono::system_clock::now() + std::chrono::seconds(1));

// B:
cv.wait_until(lk, std::chrono::steady_clock::now() + std::chrono::seconds(1));
```

<details>
<summary>答案与复习指引</summary>

**B 更好**。`steady_clock` 是单调递增的——不受 NTP 时间调整影响。`system_clock` 可能被 NTP 向前/向后调整，导致超时不可预测。

HFT 场景中 `system_clock` 调整会导致超时提前或延后——`steady_clock` 保证超时精确。

复习：超时计算永远用 `steady_clock`。`system_clock` 只用于显示时间。
</details>

### Q3: 下列代码输出什么？

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

std::unique_lock<std::mutex> lk(m);
auto start = std::chrono::steady_clock::now();
cv.wait_for(lk, std::chrono::milliseconds(500), [&]{ return ready; });
auto end = std::chrono::steady_clock::now();

auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
std::cout << ms;  // 大约多少？
```

<details>
<summary>答案与复习指引</summary>

**约 500**。`ready` 永远是 false（没人设为 true），所以 `wait_for` 超时返回。等待时间约 500ms（可能有几毫秒的调度误差）。

如果 `ready` 被设为 true，返回时间取决于何时被通知——可能 < 500ms。

复习：`wait_for` 超时后返回 false，等待时间 ≈ 指定超时。
</details>

### Q4: 为什么 HFT 不用 wait_for 做超时？

<details>
<summary>答案与复习指引</summary>

`wait_for` 内部使用 `condition_variable` 的 park/unpark 机制——涉及系统调用和上下文切换（~1-10μs）。

HFT 热路径的超时检查用自旋：
```cpp
auto deadline = std::chrono::steady_clock::now() + std::chrono::microseconds(100);
while (std::chrono::steady_clock::now() < deadline) {
    if (ready.load(std::memory_order_acquire)) break;
    _mm_pause();  // 或 yield
}
```

延迟 ~10-100ns，无系统调用。但自旋浪费 CPU——适合短超时（微秒级）。

复习：`wait_for` 适合长超时（毫秒+），自旋适合短超时（微秒级）。HFT 热路径用自旋。
</details>

---

## 参考与延伸

- 下一节：[4.6 C++20 新同步原语](06-cpp20-primitives.md)
- 回到：[第 4 章](README.md)
