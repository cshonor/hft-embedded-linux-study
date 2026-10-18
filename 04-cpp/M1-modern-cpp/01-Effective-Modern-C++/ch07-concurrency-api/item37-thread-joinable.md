# Item 37：让 std::thread 在所有路径都不可联结（joinable）

> 第 7 章 并发 API · Item 37 · 上一节：[Item 36 启动策略](item36-launch-policy.md) · 下一节：[Item 38 句柄析构行为](item38-handle-destruction.md)

## 为什么要学这个（先建立直觉）

C 程序员用 `pthread` 时，忘了 `pthread_join` 只是资源泄漏——进程不会崩，顶多少几个线程槽：

```c
pthread_t tid;
pthread_create(&tid, NULL, worker, NULL);
// 忘了 pthread_join → 线程资源泄漏，但进程不崩
```
C++ 的 `std::thread` 把这条"忘了也没关系"的路径直接封死了：**只要 `thread` 对象析构时还处于 `joinable` 状态（既没 `join` 也没 `detach`），标准规定调用 `std::terminate`——整个进程当场死亡**，不只是这个线程出事。

这不是标准委员会吓唬人。没人 join 的线程几乎必然意味着：函数返回后它还在访问已销毁的栈变量，或在你以为"程序已干净退出"时还在写全局状态。与其让你悄悄拿到 UB 定时炸弹，不如当场炸给你看。核心纪律一句话：**`std::thread` 析构前必须 unjoinable**——`join()`、`detach()`、move 走、或没启动过，四选一。难点在"所有路径"：正常 return 你记得 join，异常路径、中间 return 呢？

---

## 这节讲什么

`joinable` 的精确定义；**本机实测** joinable 析构 → `std::terminate` 的真实报错与退出码；五种安全写法逐条跑通（join / detach / 默认构造 / move / RAII 守卫）；RAII 析构的两种策略（join 派 vs detach 派）与各自风险。

---

## joinable 的精确定义

一个 `std::thread` 对象 `joinable() == true`，当且仅当它**关联着一个活跃的、尚未结束的执行线程**。以下四种情况 `joinable()` 为 `false`：

| 状态 | `joinable()` | 析构行为 |
|------|--------------|----------|
| 关联了活跃线程，未 join 未 detach | `true` | **`std::terminate`** |
| 已 `join()` | `false` | 安全 |
| 已 `detach()` | `false` | 安全 |
| 默认构造（从未关联线程） | `false` | 安全 |
| 被 move 走（所有权已转移） | `false` | 安全 |

析构规则一句话：**析构时 `joinable()` 为真 → terminate；为假 → 什么都不做**。注意 `join()`/`detach()` 对已 unjoinable 的线程调用同样是错误（抛 `std::system_error`），所以守卫代码里必须先判断 `joinable()`。

---

## 实测 1：joinable 析构 → terminate（本机实测 clang 23.1.0）

```cpp
#include <thread>
#include <cstdio>

void work() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main() {
    std::puts("main: creating thread...");
    {
        std::thread t(work);
        std::puts("main: leaving scope WITHOUT join/detach");
    }  // t 析构时仍 joinable → std::terminate
    std::puts("main: you should never see this line");
    return 0;
}
```

clang 23.1.0 + `-std=c++17 -Wall` 编译零警告，运行结果：

```
main: creating thread...
main: leaving scope WITHOUT join/detach
libc++abi: terminating
exit code: 134
```

最后一行 `you should never see this line` 果然没打印。退出码 **134 = 128 + 6**，即进程收到 **SIGABRT** 自杀——`std::terminate` 默认调用 `std::abort`，libc++ 在 stderr 留下一句 `libc++abi: terminating` 就算遗言了。注意它**不会**告诉你"是哪个 thread 没 join"，生产环境排查只能靠 core dump 或日志。

---

## 实测 2：五条安全路径逐一验证（本机实测 clang 23.1.0）

```cpp
#include <thread>
#include <cstdio>

void work(int id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::printf("worker %d done\n", id);
}

// RAII 守卫：析构里 join（等线程跑完才放行）
class ThreadGuardJoin {
    std::thread t_;
public:
    explicit ThreadGuardJoin(std::thread&& t) : t_(std::move(t)) {}
    ~ThreadGuardJoin() { if (t_.joinable()) t_.join(); }
    ThreadGuardJoin(const ThreadGuardJoin&) = delete;
    ThreadGuardJoin& operator=(const ThreadGuardJoin&) = delete;
};

int main() {
    {   // 路径 1：join 后析构
        std::thread t(work, 1);
        std::printf("before join: joinable() = %d\n", t.joinable());
        t.join();
        std::printf("after join:  joinable() = %d\n", t.joinable());
    }
    std::puts("path 1 (join): survived");

    {   // 路径 2：detach 后析构
        std::thread t(work, 2);
        t.detach();
        std::printf("after detach: joinable() = %d\n", t.joinable());
    }
    std::puts("path 2 (detach): survived");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    {   // 路径 3：默认构造，从未关联线程
        std::thread t;
        std::printf("default ctor: joinable() = %d\n", t.joinable());
    }
    std::puts("path 3 (default): survived");

    {   // 路径 4：move 转移所有权，源对象不再 joinable
        std::thread t1(work, 3);
        std::thread t2 = std::move(t1);
        std::printf("after move: t1.joinable() = %d, t2.joinable() = %d\n",
                    t1.joinable(), t2.joinable());
        t2.join();
    }
    std::puts("path 4 (move): survived");

    {   // 路径 5：RAII 守卫，离开作用域自动 join
        ThreadGuardJoin g(std::thread(work, 4));
        std::puts("guard scope: about to leave");
    }
    std::puts("path 5 (RAII guard): survived");
    return 0;
}
```

实测输出（关键看 `joinable()` 的翻转时机）：

```
before join: joinable() = 1
worker 1 done
after join:  joinable() = 0
path 1 (join): survived
after detach: joinable() = 0
path 2 (detach): survived
worker 2 done
default ctor: joinable() = 0
path 3 (default): survived
after move: t1.joinable() = 0, t2.joinable() = 1
worker 3 done
path 4 (move): survived
guard scope: about to leave
worker 4 done
path 5 (RAII guard): survived
exit code: 0
```

三条要点被输出直接证实：`join()`/`detach()` 后 `joinable()` 立即变 0；move 之后**源对象** `t1` 变 0、目标 `t2` 为 1（责任跟着所有权走）；RAII 守卫里 worker 4 的 `done` 打印在 `path 5 survived` **之前**——说明析构里的 `join` 是真的在等线程跑完。

---

## RAII 守卫：join 派 vs detach 派

`ThreadGuard` 析构里到底调 `join()` 还是 `detach()`？这是两种设计哲学：

**join 派（等线程跑完）：** 安全、语义确定，但析构会**阻塞**——线程是死循环或长任务时守卫所在作用域退不出去，异常栈展开期间阻塞尤其危险。

**detach 派（放手不管）：** 不阻塞，但线程完全失控——若按引用捕获了局部变量，守卫放行后线程访问的是已销毁的栈内存，UB。

**C++20 的标准答案：** 标准库替你做了选择——`std::jthread` 析构时先 `request_stop()` 再 `join()`，线程体只要定期检查 `std::stop_token` 就能被协作式打断，兼顾了"不泄漏"和"不死等"。C++17 及以前就自己写 `ThreadGuard`，绝大多数场景选 join 派。

---

## 完整可运行例子

"实测 2"即完整可运行例子（五条路径 + RAII 守卫 + `joinable()` 观察），输出见上；最小复现 terminate 用"实测 1"。

---

## 常见错误（新手踩坑）

**错误 1：异常路径忘了 join**

```cpp
void process() {
    std::thread t(work);
    if (error) throw std::runtime_error("error");  // 栈展开 → t 析构仍 joinable → terminate
    t.join();   // 只有正常路径能走到这
}
```

**修正：** 用 RAII 守卫（或 C++20 `std::jthread`），析构自动 join，异常路径也安全。

**错误 2：detach 后线程引用局部变量**

```cpp
void start() {
    int id = 42;
    std::thread t([&id]{ use(id); });  // 按引用捕获 id
    t.detach();                        // start 返回后 id 销毁 → 悬垂引用 → UB
}
```

**修正：** 按值捕获 `[id]`，或用 `shared_ptr` 共享生命周期，或干脆别 detach。

**错误 3：对已 unjoinable 的线程再 join**

```cpp
std::thread t(work);
t.join();
t.join();   // joinable() 已为 false → 抛 std::system_error
```

**修正：** 调 `join()`/`detach()` 前先判 `joinable()`——这正是守卫类析构里那行 `if` 存在的原因。

**错误 4：守卫类允许拷贝，导致责任不唯一**

```cpp
ThreadGuard g1(std::thread(work));
ThreadGuard g2 = g1;   // 若能拷贝：两个守卫管同一个线程，析构 join 两次 → UB
```

**修正：** `= delete` 拷贝构造与拷贝赋值，只允许 move（责任随所有权唯一转移）。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 线程创建 | `pthread_create` | `std::thread` | RAII 对象，不是裸 ID |
| 忘了 join | 资源泄漏 | **`terminate` 进程崩溃** | C++ 强制生命周期纪律 |
| 异常路径清理 | 手动 `pthread_cleanup_*` | RAII 守卫析构自动 join | 栈展开也会触发析构 |
| 所有权 | `pthread_t` 随便拷 | `thread` 只能 move | 一个线程一个责任人 |

**一句话总结：** `std::thread` 析构前必须 unjoinable，RAII 守卫（或 C++20 `std::jthread`）是覆盖异常路径的标准做法。

---

## HFT 关联

- **守护进程崩溃纪律**：任何一条异常路径漏掉 join，后果不是泄漏而是**整个交易进程 terminate**——风控、行情线程的创建处一律 RAII 化。
- **热路径不 join**：行情回调里绝不创建需 join 的短线程（join 阻塞 = 延迟抖动）；要么线程池预创建，要么 detach 但严格按值捕获。
- **退出码监控**：进程以 134（SIGABRT）退出且日志有 `terminating`，优先怀疑 `std::thread` 的 terminate，而非段错误（139）。

---

## 自测题

**1. `std::thread` 析构时仍 joinable 会发生什么？退出码大概是什么样？**

<details><summary>答案</summary>
调用 `std::terminate`，默认进而调用 `std::abort`，整个进程以 SIGABRT 终止。本机实测退出码 134（128+6），stderr 输出 `libc++abi: terminating`。不会自动帮你 join，也不会只杀那个线程。
</details>

**2. 哪四种情况下 `joinable()` 为 false？**

<details><summary>答案</summary>
已 `join()`、已 `detach()`、默认构造从未关联线程、被 move 走（所有权已转移给另一个 `thread` 对象）。这四种状态下析构都是安全的。
</details>

**3. 为什么 `ThreadGuard` 析构里写的是 `if (t.joinable()) t.join();` 而不是直接 `t.join();`？**

<details><summary>答案</summary>
对 unjoinable 的线程调 `join()` 会抛 `std::system_error`（invalid argument）。守卫对象可能被 move 过、或构造时拿到了空 thread，析构里必须先判 `joinable()`。而且析构函数里再抛异常（栈展开中）会直接 terminate，所以这层保护是双重的。
</details>

**4. RAII 守卫析构里选 join 还是 detach，各自的风险是什么？**

<details><summary>答案</summary>
join 派：析构阻塞，线程不结束作用域就退不出去（死循环线程 = 挂死），但语义安全。detach 派：不阻塞，但线程失控，若按引用捕获了局部变量，守卫放行后线程访问的是已销毁内存（UB）。C++20 `std::jthread` 用 `request_stop()` + join 的协作式取消给出了标准答案。
</details>

**5. move 一个 `std::thread` 之后，源对象析构安全吗？实测中 `t1`/`t2` 的 `joinable()` 各是多少？**

<details><summary>答案</summary>
安全。实测输出 `after move: t1.joinable() = 0, t2.joinable() = 1`——所有权（和 join 责任）转移到 `t2`，源对象 `t1` 变为 unjoinable，析构什么都不做。
</details>

**6. 下面代码有什么问题？**

```cpp
void work() {
    int data = 42;
    std::thread t([&data]{ process(data); });
    t.detach();
}
```

<details><summary>答案</summary>
`detach()` 后 `work()` 立即返回，`data` 销毁，而线程按引用捕获了它——悬垂引用，UB。修正按推荐顺序：① 按值捕获 `[data]`；② 改用 RAII/jthread 让 `work()` 等线程结束；③ 数据用 `shared_ptr` 共享生命周期。
</details>

---

## 参考与延伸

- 上一节：[Item 36 启动策略](item36-launch-policy.md) · 下一节：[Item 38 句柄析构行为](item38-handle-destruction.md)
- 回到：[第 7 章 并发 API](README.md)
