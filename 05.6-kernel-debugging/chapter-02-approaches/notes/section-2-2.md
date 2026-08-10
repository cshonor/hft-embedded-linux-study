# 2.2 仪表化方法概览

> ⬜ 跳读

## 本节要点

| 方法 | 工具 | 侵入性 | 适用阶段 |
|------|------|--------|---------|
| 打印 | printk | 高 | 开发 |
| 动态调试 | dyndbg | 低 | 开发/测试 |
| 探针 | kprobes | 中 | 开发/生产 |
| 追踪 | ftrace | 低 | 开发/生产 |
| eBPF | bpftrace | 极低 | 生产 |
| 内存检测 | KASAN/KFENCE | 高/低 | 开发/生产 |

## 工具选择原则

1. 先用 ftrace（无侵入）了解调用流程
2. 再用 kprobes 在关键点捕获数据
3. 必要时用 printk 精确定位
4. 用 KASAN/KCSAN 系统性检测内存/并发问题

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么推荐先用 ftrace 而非 printk 调试？

> ftrace 无侵入——不改变时序、不需重编译、可动态开关。printk 会序列化输出改变时序（可能掩盖竞争条件），且需要重新编译代码修改打印点。先用 ftrace 了解整体调用流程，再用 kprobes/printk 在精确定位后深入。

</details>
