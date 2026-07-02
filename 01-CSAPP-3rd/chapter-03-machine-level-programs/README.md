# Ch 3 程序的机器级表示 · Machine-Level Representation of Programs

> **CSAPP 3rd** · Bryant & O'Neill · **选读 🟡**（Part I）

> 本章定位：**读懂 x86-64 汇编** — C 控制流、函数调用、栈帧、结构体布局、缓冲区溢出。HFT 不必写汇编，但 **perf 火焰图、反汇编热路径、ABI/对齐** 都建立在本章。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 3.1–3.2 历史观点与程序编码 | [notes/section-3.1-3.2-历史观点与程序编码.md](./notes/section-3.1-3.2-历史观点与程序编码.md) |
| 3.3–3.4 数据格式与访问信息 | [notes/section-3.3-3.4-数据格式与访问信息.md](./notes/section-3.3-3.4-数据格式与访问信息.md) |
| 3.5 算术和逻辑操作 | [notes/section-3.5-算术与逻辑操作.md](./notes/section-3.5-算术与逻辑操作.md) |
| 3.6 控制（条件码、分支、循环、switch） | [notes/section-3.6-控制流.md](./notes/section-3.6-控制流.md) |
| 3.7 过程（栈、调用约定、递归） | [notes/section-3.7-过程与栈帧.md](./notes/section-3.7-过程与栈帧.md) |
| 3.8 数组分配和访问 | [notes/section-3.8-数组与指针运算.md](./notes/section-3.8-数组与指针运算.md) · [02 指针步长](../../../02-c-programming/notes/pointer-arithmetic-and-stride.md) |
| 3.9 结构、联合与对齐 | [notes/section-3.9-结构体联合与对齐.md](./notes/section-3.9-结构体联合与对齐.md) |
| 3.10 指针、gdb 与缓冲区溢出 | [notes/section-3.10-指针调试与缓冲区溢出.md](./notes/section-3.10-指针调试与缓冲区溢出.md) |
| 3.11 浮点代码 | [notes/section-3.11-浮点代码.md](./notes/section-3.11-浮点代码.md) |

---

## 大白话 · 本章一条线

> **C 编译成机器码后，长什么样？函数怎么调？栈上放了什么？**

```
C 源码
  → gcc -S / objdump -d
  → mov / lea / cmp / jxx / call / ret
  → 寄存器传参 + 栈帧 + 返回地址
```

**HFT 三个实用出口：**

1. **读热路径汇编** — `perf annotate`、确认是否多了分支/内存 load（→ 3.5–3.6）
2. **懂调用约定** — 哪些寄存器 callee-saved、参数在哪，写内联汇编 / FFI 不错位（→ 3.7）
3. **数据结构布局** — `struct` padding、cache line、协议 struct 与 ABI 对齐（→ 3.8–3.9）

---

## 本章 Checklist

- [ ] 会用 `gcc -S`、`objdump -d`；认识 AT&T 与 Intel 汇编格式差异
- [ ] 说出 x86-64 整数参数寄存器顺序：`%rdi, %rsi, %rdx, %rcx, %r8, %r9`
- [ ] 解释 `%rsp`、栈向下增长、`push`/`pop`、返回地址在栈上的位置
- [ ] 读懂 `cmp` + `jne`/`jg` 与条件码 `ZF/SF/OF/CF`
- [ ] 说明 **cmov** vs 分支 — 与分支预测、HFT 热路径的关系
- [ ] 画出简单函数的栈帧：`call` 压返回地址、局部变量、`leave`/`ret`
- [ ] 计算带 padding 的 `struct` 大小；理解对齐与 **false sharing**
- [ ] 简述缓冲区溢出、canary、NX、ASLR 各防什么

---

## HFT 精读捷径

```
必读：3.4 传送/栈 · 3.6 控制与 cmov · 3.7 调用约定 · 3.9 对齐
热路径 profile：3.5 lea、3.6 分支 · 配合 Ch 5 优化
协议 struct：3.8–3.9
安全/运维：3.10 扫读；3.11 浮点路径按需
3.1 历史、3.2.3 格式注解 — 可跳过
```

---

## 相关章节

- 上一章：[../chapter-02-representing-information/](../chapter-02-representing-information/)
- 下一章：[../chapter-04-processor-architecture/](../chapter-04-processor-architecture/)
- 优化：[../chapter-05-optimizing-performance/](../chapter-05-optimizing-performance/)
- perf 读栈：[14-Systems-Performance Ch 13](../../15-Systems-Performance-2nd/chapter-13-perf/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
