# 条款 48：了解 new-handler 的行为

## 本节讲什么

**Understand the behavior of the new-handler.** `operator new` 分配失败时不是
直接抛 `bad_alloc`——它先调用 **new-handler**（`std::set_new_handler` 注册），
给你"抢救内存"的机会：释放应急池、重试、或者干脆换种死法。
这条讲清 new-handler 的循环语义、注册方法、以及工程上该不该用它。

← 上一条 [item47 traits classes](../ch07-templates-generics/item47-请使用traits-classes表现类型信息.md)；
下一条 [item49 类内 new/delete](./item49-了解class-specific的new-delete.md)。

---

## 1. new 失败时的完整决策链

```
operator new 分配失败
  → 调用 new-handler（若已注册）
      → handler 选择：
          ① 释放内存（如应急池）→ **重试分配**（循环！成功就返回）
          ② 注册另一个 new-handler（下次失败用新的）→ 重试
          ③ 注销 new-handler（set_new_handler(nullptr)）→ 重试 → 抛 bad_alloc
          ④ 自己抛 bad_alloc（或派生类异常）
          ⑤ 不返回（abort/exit/log 后终止）
  → 未注册 handler 或 handler 选择③ → 抛 std::bad_alloc
```

**关键点**：new-handler 在**循环**里被调用——它若返回，分配会**重试**；
只有 handler 抛异常/不返回/被注销，失败才最终落地为 `bad_alloc`。

## 2. 注册与使用（最小骨架）

```cpp
#include <new>
#include <cstdlib>
#include <iostream>

void emergency_handler() {
    static bool once = [] {
        std::cerr << "内存耗尽：释放应急池并准备退出\n";
        // release_emergency_pool();   // 做法一：释放预留内存，让重试成功
        return true;
    }();
    // 做法二：抢救一次后终止（常见工程选择：宁可死得体面，不跑在缺氧状态）
    std::abort();
}

int main() {
    std::set_new_handler(emergency_handler);
    // ...
}
```

**设计纪律**：
- handler 里**别分配内存**（栈上日志/固定缓冲可以——再 new 就是递归失败）
- 老代码的 `set_new_handler` 返回旧 handler——需要链式保存恢复时自己接住
- C 时代的 `set_new_handler(0)` + 检查返回值模式 → 现代写法用 `new (std::nothrow)`
  （→ 19.1 ④）或直接用返回 optional 的分配函数

## 3. 工程上该不该用（诚实评估）

**适合**：
- **应急池**（emergency reserve）：启动时预留一块内存，OOM 时释放它换取
  "有序降级"的时间窗（写完日志、平掉仓位、保存状态再死）——
  这是 new-handler 唯一被反复验证的工业用法
- 嵌入式/无异常环境的最后防线

**不适合**：
- 指望它"解决"内存不足——handler 治标（释放点边角料），
  真正的内存治理在预算/池化/监控（→ 06.6.5 ch07）
- 库代码里注册全局 handler——它是**进程全局**的，库之间互相覆盖是灾难
  （谁的 handler 生效取决于注册顺序，库 B 把库 A 的 handler 顶掉了）

## HFT 关联

- **交易系统的 OOM 预案**就是这个模式：启动时 `malloc` 一块应急池锁在内存里，
  new-handler 触发时 → 释放应急池 + **先撤单/平仓**（比活着重要）→ 落盘状态 → 退出——
  "内存耗尽时的行为"必须和"断线时的行为"一样被显式设计
- 热路径库 `-fno-exceptions` 时 bad_alloc 路径根本不存在：
  运行期零分配的纪律（→ 19.1 HFT）让 new-handler 沦为纯冷路径设施
- 全局 handler 与 LD_PRELOAD 分配器（jemalloc/tcmalloc）的相互作用要在
  集成测试里过一遍——替换 operator new 的分配器可能改变失败语义

## 代码自测

**题目 1：** new-handler 返回后会发生什么？为什么它是"循环"？

<details>
<summary>参考答案</summary>

new-handler 返回后，`operator new` **重试分配**——若仍失败，**再次调用**
new-handler……如此循环，直到：① 某次分配成功（handler 释放的内存生效了）；
② handler 抛异常（bad_alloc 落地）；③ handler 被注销（默认抛 bad_alloc）；
④ handler 不返回（abort/exit）。
所以 handler 的每个动作都必须"让下一次重试更有可能成功"或"终结循环"——
一个什么都不做就返回的 handler = **死循环**（分配永远失败，handler 永远被调）。

</details>

**题目 2：** 为什么库不应该注册 new-handler？

<details>
<summary>参考答案</summary>

new-handler 是**进程级全局状态**：`set_new_handler` 直接覆盖前一个——
库 A 注册的 OOM 应急逻辑，会被库 B 的注册**静默顶掉**（没有链式、没有警告）。
两个库都"为自己好"注册 handler，结果是后加载的赢、先加载的失效——
而且失效方完全不知道自己被顶了。
handler 的注册权属于**应用层**（main 的早期）——库要抢救内存，
用自己的内部分配器/池，别碰全局钩子。
（同理的全局状态还有：errno 策略、信号 handler、locale——库的禁区清单。）

</details>

**题目 3：** 设计交易系统"内存耗尽应急预案"，new-handler 里的动作顺序应该是什么？

<details>
<summary>参考答案</summary>

按"活着的价值排序"：
① **释放应急池**（启动时预留的那块）——让后续抢救动作有内存可用；
② **撤单/平仓指令**（比状态落盘优先——留在场上的敞口是最大风险，
   注意这些指令通道不能依赖堆分配，要预分配好）；
③ **关键状态落盘**（持仓/订单簿快照——固定缓冲区写，不调 new）；
④ **告警**（带外通道：独立 socket/信号量，不走应用内日志队列）；
⑤ **终止**（abort——core dump 留现场）。
全程禁令：handler 路径上**零堆分配**——所有抢救资源必须启动时预分配。
这套顺序的本质：OOM 时"先控制风险，再保存证据，最后死"——
和断线预案是同构的风险哲学。

</details>
