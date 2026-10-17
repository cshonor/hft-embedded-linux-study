# 条款 51：编写 new 和 delete 时需固守常规

## 本节讲什么

**Adhere to convention when writing operator new and operator delete.**
替换 new/delete 不只是"调 malloc 包一层"——语言对这组函数有一套
**行为常规**（convention），违反任意一条都会让你的重载变成
与用户代码的隐性冲突。本条列出全部常规与每条背后的合约。

← 上一条 [item50 替换时机](./item50-了解new和delete的合理替换时机.md)；
下一条 [item52 placement new/delete 配对](./item52-写了placement-new也要写placement-delete.md)。

---

## 1. operator new 的五条常规

```cpp
void* operator new(std::size_t size) {
    if (size == 0) size = 1;                 // 常规①：0 字节请求按 1 处理
    while (true) {                           // 常规②：无限循环重试
        void* p = my_alloc(size);
        if (p) return p;
        std::new_handler h = std::get_new_handler();   // 常规③：失败先调 new-handler
        if (h) h();                          // （→ item48 的决策链）
        else throw std::bad_alloc();         // 常规④：无 handler 抛 bad_alloc
    }
}
```

| 常规 | 合约 |
|---|---|
| ① 0 字节也返回**合法可 delete 的指针** | `new char[0]` 必须成功（标准允许但要求行为正确） |
| ② 失败→handler→**重试**的循环 | handler 可能释放内存，重试才有意义（→ item48 循环语义） |
| ③ 失败先调 `new_handler` | 跳过它 = 绕过应用的 OOM 预案（应急池失效，→ item48 HFT） |
| ④ 最终失败抛 `std::bad_alloc` | 用户代码 catch 的就是它——返回 nullptr 是非标准（nothrow 版除外） |
| ⑤ 返回值按默认**对齐** | 至少 `max_align_t` 对齐（C++17 起超对齐走 align_val_t 版） |

## 2. operator delete 的三条常规

```cpp
void operator delete(void* p) noexcept {
    if (!p) return;          // 常规①：delete nullptr 必须是无害 no-op
    my_free(p);              // 常规②：释放的必须是"自己分配器给出去的指针"
}
// 常规③：noexcept——delete 不许抛（析构路径上抛异常 = terminate，→ 18.1 ②）
```

- **delete nullptr 合法且必须什么都不做**——`if (p) delete p;` 的判空是用户的多余动作，
  你的 delete 自己就得抗住 nullptr
- **配对原则**：operator new 从哪拿（malloc/池/aligned_alloc），
  operator delete 就还哪去——混用（new 池化、delete 走 free）= 堆损坏
- C++14 起记得**sized delete 也写**（→ 19.1 ②）——编译器在尺寸已知时优先选它，
  只写非 sized 版会错过池化红利（且两个版本逻辑必须一致）

## 3. 类内重载的额外常规（item49 的补全）

- 类内 `operator new` 会**隐藏**全局的所有重载形式（含 placement new）——
  要用 placement 必须类内同样声明（→ item52 的配对）
- 数组版 `operator new[]/delete[]` **独立于**普通版——
  只重载普通版，`new T[n]` 静默走全局（常见泄漏源：数组绕过池）
- 继承传播（item49 ②）：派生类尺寸可能更大，重载要么按最大尺寸设计、
  要么派生类自己重写

## HFT 关联

- 池分配器的"operator new 常规"在池语境的映射：池空了的语义必须显式定义——
  返回 nullptr（调用方判空）还是 terminate（启动时按峰值预分配，池空=bug）——
  HFT 选后者：**运行期池空是容量规划事故，应当场爆炸而不是静默降级**
- `noexcept` 的 delete 与 `-fno-exceptions` 库天然兼容——
  池化世界里 delete 路径本来就该是纯 freelist 归还
- 统计版重载（item50 ②）别忘了它自己也得守全部常规——
  观测代码在分配路径上，一次违规递归分配就是死锁/递归崩溃

## 代码自测

**题目 1：** `operator new` 的失败处理为什么是"循环 + new-handler"而不是直接抛异常？

<details>
<summary>参考答案</summary>

因为 **new-handler 存在的目的就是"抢救后重试"**（→ item48 决策链）：
应急池释放、GC 触发、缓存清页——这些动作只有"重试分配"才有意义。
直接抛异常等于宣布"handler 白注册了"：应用的 OOM 预案（item48 的应急池）
被你的重载静默绕过。
所以标准形态是 `while (true) { 尝试; 失败 → 调 handler; 再试; }`——
handler 选择终止循环（抛异常/abort/注销自己）时，bad_alloc 才落地。
你的重载可以不知道 handler 干什么，但必须**把循环留给它**。

</details>

**题目 2：** 为什么 `delete nullptr` 必须是 no-op？用户写 `if (p) delete p;` 不是更清楚吗？

<details>
<summary>参考答案</summary>

因为**语言保证 delete 接受 nullptr**——所有标准容器、智能指针、
以及不写判空的既有用户代码都依赖这个约定。
你的自定义 operator delete 如果对 nullptr 不免疫（比如池的 release()
拿 nullptr 查归属池时崩溃），破坏的是**所有调用方**，不只是"写判空的谨慎者"。
反过来：`if (p) delete p;` 在用户侧永远是多余的——
写它是无害的风格问题，delete 实现不抗 nullptr 是**正确性 bug**。
常规的存在就是为了"依赖者众多"的行为不被单个实现破坏。

</details>

**题目 3：** 类内重载了 `operator new` 后，`new (buf) T`（placement new）为什么编译失败？

<details>
<summary>参考答案</summary>

类内声明任何 `operator new` 都触发**名称遮掩**（item33 同款规则）——
类作用域里的 operator new 把全局的**全部**重载形式藏起来，
包括全局的 placement new（`void* operator new(size_t, void*)`）。
用户写 `new (buf) T` 时，候选集里只剩你类内的那版（签名不符）→ 编译错误。
解法：类内同时声明 placement 版本——
`static void* operator new(std::size_t, void* p) noexcept { return p; }`。
这正是 item52 的入口：placement new/delete 的配对常规，
从"别让类内重载把 placement 藏掉"开始。

</details>
