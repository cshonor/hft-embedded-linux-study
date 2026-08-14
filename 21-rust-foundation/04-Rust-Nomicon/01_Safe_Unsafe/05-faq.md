# 五 · 高频易错疑问

← [本章目录](./README.md) · 上一节：[04-trust-and-nonlocality.md](./04-trust-and-nonlocality.md) · 下一节：[06-hft-practice.md](./06-hft-practice.md)

---

## 疑问 1：unsafe 是修饰变量吗？接收 unsafe 函数返回值需要 unsafe 变量吗？

**不是。** `unsafe` 只约束**操作行为**，不约束变量类型。

调用 unsafe 函数得到的裸指针、句柄等，可直接存放在普通 Safe 变量中；只有对这个变量执行 5 类高危操作时，才需要 unsafe 块包裹。

## 疑问 2：unsafe trait 代表内部所有方法都是 unsafe 吗？

**完全无关。**

- `unsafe trait`：约束**实现该特征的类型**，实现时要加 `unsafe impl`；
- 普通 trait 内部可以单独定义 `unsafe fn` 方法，trait 本身仍是 Safe trait。

二者互不影响。

## 疑问 3：Unsafe 是不是抛弃所有权、借用规则，变回 C 语言？

**不是。** 所有权、生命周期、借用规则依然是底层约束，只是编译器不再主动校验。

写 unsafe 代码依然要遵循独占可变、内存有效、谁分配谁释放等所有权逻辑，只是全部需要人工自查，违背规则照样 UB。

## 疑问 4：既然 unsafe 有风险，为什么不直接删掉？

Rust 需要对接操作系统、C 库、网卡硬件、内核旁路（XDP/DPDK），这些场景本身不受 Rust 类型系统管控，必须提供绕过编译检查的通道。

unsafe 是系统编程、高性能 HFT 开发的刚需，只是严格限制使用范围。

→ 仓库对照：[RFR Ch09 Unsafe](../../02-RFR/Chapter-09-Unsafe-Code/README.md) · [ER Item 16](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/README.md)
