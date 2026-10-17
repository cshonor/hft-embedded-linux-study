# 条款 11：杜绝资源管理函数内部发生异常造成泄漏

## 本节讲什么

**Prevent exceptions from leaving resource-management functions.**
资源管理函数（分配/释放/加锁/解锁）是异常安全的**最后防线**——
它们自己抛异常，泄漏的就是"正在管理中"的资源。
本条处理三类高危函数：deleter、dispose/cleanup 类、以及"先申请后操作再归还"的流程函数。

← 上一条 [item10 构造函数异常](./item10-构造函数抛出异常时，如何防止内存资源泄漏.md)；
下一条 [item12 抛异常的对象拷贝](./item12-抛出异常时，理解对象拷贝的完整流程与开销.md)。

---

## 1. 三类高危函数与各自的坑

**坑一：deleter/释放函数抛异常**

```cpp
void release(Resource* r) {
    log_release(r);      // ← 这行抛异常（日志队列满）——
    delete r;            //    delete 到不了：r 泄漏
}
```

释放路径上的任何操作都可能让"释放本身"完不成——
**释放函数必须是异常-free 的**（noexcept 是上策）。

**坑二："先申请后操作再归还"的流程函数**

```cpp
void with_lock(Mutex& m) {
    m.lock();
    do_work();           // 抛异常 → unlock 永远不到
    m.unlock();
}
```

这是 item09（RAII）的直接应用域——`lock_guard` 存在就是为了这一坑。

**坑三：清理代码依赖"当前没人在抛异常"**

栈展开中调用清理函数，清理函数里再抛 → `std::terminate`（18.1 ②）——
清理路径必须 noexcept 免疫（`catch(...)` 吞掉 + 记录）。

## 2. 三条防线

| 防线 | 做法 |
|---|---|
| ① 资源管理函数标 `noexcept` | 编译期声明 + review 聚焦（→ item25 swap、18.1 析构同理） |
| ② 释放逻辑 RAII 化 | unique_ptr/lock_guard/Guard——释放不依赖执行流（item09） |
| ③ 清理路径的异常吞没 | `try { cleanup(); } catch (...) { /* 记带外日志 */ }`——清理失败 ≠ 新异常 |

**noexcept 的连锁效应**：标了 noexcept 的函数里调用非 noexcept 的函数，
编译器**不**报错（语义检查是运行时的——真抛了就 terminate）——
所以 noexcept 是**契约声明**，配套的是 review 时逐行确认调用链。

## 3. 自查：你的资源管理函数能过这三问吗

```text
问①：释放/解锁/归还动作依赖"执行流到达某行"吗？→ 是则 RAII 化
问②：管理函数里调用的每个函数都 noexcept 吗？→ 不确定则 try-catch 包
问③：它可能在栈展开中被调用吗（析构/catch 块里）？→ 是则必须绝不抛
```

## HFT 关联

- 池的 `release()` 必须 noexcept（→ item52 的 placement delete、
  item49 的池语义）——归还槽位的路径上任何异常都是 freelist 流血
- 风控前置的锁/网卡模式切换这类"系统级资源"：管理函数异常 =
  交易系统处于**未知状态**（锁没解？模式没切回？）——比泄漏更可怕的是
  "资源状态不确定"，这类函数的错误处理要显式到返回值/错误码
- 与 item09 的分工：item09 管"业务函数里别泄漏"，本条管"**管理函数自己**
  别成为泄漏源"——两层都守住，异常安全才闭合（→ Effective item29 的三级保证）

## 代码自测

**题目 1：** 为什么 deleter/释放函数内部的异常比业务函数的异常更危险？

<details>
<summary>参考答案</summary>

业务函数的异常代价是**本次操作失败**（资源有 RAII 兜底，item09）；
释放函数的异常代价是**资源永久泄漏 + 状态不确定**：
① 释放动作本身没完成（delete 没执行到——申请的资源永远挂着）；
② 它常在栈展开/析构路径上被调用（18.1 ②）——
此时抛异常直接 `std::terminate`，连"泄漏后继续跑"的机会都没有。
所以资源管理函数是异常安全的最后防线：它们必须 noexcept 化，
内部任何可疑调用都要 try-catch 吞掉（失败转带外日志，不转新异常）。

</details>

**题目 2：** `noexcept` 标上之后，编译器会检查函数体内所有调用都不抛吗？

<details>
<summary>参考答案</summary>

**不会**。noexcept 是**运行时的契约**不是编译期的证明：
函数体内调用非 noexcept 的函数，编译器一声不吭——
运行期真的抛出来，当场 `std::terminate`（没有展开、没有 catch 机会）。
所以 noexcept 的正确用法是"**声明 + 人工确认调用链**"：
标之前逐行过函数体（调用的每个函数都 noexcept 吗？分配吗？），
配 `static_assert(noexcept(expr))` 钉住关键调用（→ item25 ② 的实测手法）。
把 noexcept 当"编译器会帮我验证"是最大的误解——它验的是
"你承诺了，违约就 terminate"，不是"你写对了"。

</details>

**题目 3：** 给池写 `release(void* p) noexcept`，内部实现要做哪三件事？

<details>
<summary>参考答案</summary>

① **nullptr 免疫**（→ Effective item51 常规：`if (!p) return;`——
delete 语义对齐，空调用无害）；
② **归还逻辑纯无锁/无分配**（freelist 头插法这类 O(1) 无分配操作——
分配异常在归还路径上没有容身之处）；
③ **可疑操作全吞**（如果非要打日志/统计，全部
`try { ... } catch (...) {}`——统计失败不值得 terminate；
更彻底的做法是统计走无锁原子计数，天然不抛）。
bonus：配 `static_assert(noexcept(release(p)))` 进单测——
让"noexcept 被意外摘掉"成为编译错误（→ item25 的验收手法）。

</details>
