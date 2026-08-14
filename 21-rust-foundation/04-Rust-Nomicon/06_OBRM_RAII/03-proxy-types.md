# 3 · 代理类型的致命危险 (Proxy Types)

← [本章目录](./README.md) · 上一节：[02-forget-leak.md](./02-forget-leak.md) · 下一节：[04-unwinding.md](./04-unwinding.md)

---

泄漏普通 `Box` 顶多浪费内存；泄漏**代理对象**可能破坏内存安全：

| 类型 | 风险 |
|------|------|
| **`vec::Drain`** | 迭代中途 `forget` → 元素状态不一致；std 用 **leak amplification** 妥协保容器基本安全 |
| **`Rc`** | 恶意 `forget` 克隆体 → 引用计数溢出 → 提前释放 → **UAF** |
| **`thread::scoped`（已移除）** | 守卫被 `forget` → 父栈销毁而子线程仍运行 → **UAF**；API 已废弃重设计 |

→ 源码：[src/proxy_types.rs](./src/proxy_types.rs)（Drain + 注释说明 Rc/scoped）
