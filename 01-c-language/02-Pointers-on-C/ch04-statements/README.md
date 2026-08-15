# 第 4 章 语句

**Statements**

## 本章讲什么

C **全部控制流**：表达式/空语句、代码块、**if**、**while/do/for**、**break/continue**、**switch**、**goto**、**return**。内核调度、DPDK 轮询、HFT 报文解析与风控分支都建立在本章。

## 学习重点

- **if 后分号**、**悬垂 else** → 强制 `{}`
- **`=` vs `==`** 在循环条件中
- **for(;;)** 数据面主循环；**while rx_burst**
- **switch** 穿透与 **default**
- **break/continue** 语义（for 中 continue 仍执行更新段）
- **goto** 仅错误清理；内核范式 vs 业务禁用

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | `for(;;)` 轮询、`while` 批量收包、continue 滤心跳 |
| 内核/驱动 | do-while 重试、goto 释放、switch 状态 |
| HFT | if 风控、switch 报文类型、switch-default |

## 实操（建议完成）

1. DPDK 风格轮询 + continue 滤心跳  
2. 悬垂 else 验证  
3. switch 多 case 共享 + 穿透  
4. goto 资源释放  
5. `while(ret=0)` 陷阱  
6. do-while 发送重试  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch02 块作用域；ch03 条件表达式 |
| 后序 | ch05 运算/短路；ch06 指针遍历；ch17 ADT 遍历 |
| 配套 | 《C陷阱与缺陷》ch01–ch03 |

## 小节

- [4.1 空语句](./4.1-空语句.md)
- [4.2 表达式语句](./4.2-表达式语句.md)
- [4.3 代码块](./4.3-代码块.md)
- [4.4 if 语句](./4.4-if语句.md)
- [4.5 while 循环](./4.5-while循环.md)
- [4.6 for 循环](./4.6-for循环.md)
- [4.7 do 循环](./4.7-do循环.md)
- [4.8 switch 语句](./4.8-switch语句.md)
- [4.9 goto 语句](./4.9-goto语句.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: if 后分号 bug

```c
int x = -5;
if (x > 0);
    printf("positive\n");
printf("done\n");
```

> 输出什么？为什么 `positive` 被打印了？

<details>
<summary>答案与复习指引</summary>

**输出：** `positive` + `done`

**解析：** `if (x > 0);` — 分号是空语句，`if` 控制空语句（什么都不做）。`printf("positive")` 不在 `if` 内，无条件执行。

**教训：** `if` 后不要写分号。用 `{}` 包裹语句体。

**复习：** → [4.2 if 语句](4.2-表达式语句.md)

</details>

### Q2: for(;;) 数据面主循环

```c
for (;;) {
    struct rte_mbuf *pkts[32];
    uint16_t n = rte_eth_rx_burst(port, 0, pkts, 32);
    if (n == 0)
        continue;
    for (int i = 0; i < n; i++)
        process_packet(pkts[i]);
}
```

> `for(;;)` 等价于什么？数据面为什么用它而不是 `while(1)`？

<details>
<summary>答案与复习指引</summary>

**答案：** `for(;;)` ≡ `while(1)` — 无限循环。

DPDK / 数据面用 `for(;;)` 因为：
1. 表达"永不退出"语义更清晰（无条件的）
2. 有些编译器对 `for(;;)` 不生成条件跳转指令（少一条 `cmp` + `jne`）
3. `while(1)` 字面上有一个条件判断 `1 != 0`（虽然编译器会优化掉）

**复习：** → [4.6 for 循环](./4.6-for循环.md)

</details>

### Q3: switch default 重要性

```c
enum cmd { CMD_START, CMD_STOP, CMD_RESET };

void handle(enum cmd c) {
    switch (c) {
        case CMD_START: start(); break;
        case CMD_STOP:  stop();  break;
        // 忘了 CMD_RESET
    }
}
handle(CMD_RESET); // 会怎样？
```

> `CMD_RESET` 会怎样？default 有什么用？

<details>
<summary>答案与复习指引</summary>

**答案：** `CMD_RESET` 不匹配任何 case → 跳过 switch，什么也不做。如果忘了 `default`，新增枚举值时编译器不会警告。

**教训：** 写 `default: assert(0);` 或 `default: log_warn("unknown cmd");` → 新增枚举值时能尽早发现问题。用 `gcc -Wall -Wswitch` 让编译器帮忙检查遗漏的 case。

**复习：** → [4.8 switch 语句](./4.8-switch语句.md)

</details>

---

## 代码自测

**题目 1：** 以下代码在 C 语言中能编译吗？说明了 C 的什么特性？
```c
#include <stdio.h>
int main() {
    printf("hello\n");
}
```

<details>
<summary>参考答案</summary>

能编译。C 是编译型语言——源代码经过预处理、编译、汇编、链接四个阶段生成可执行文件。这个程序体现了 C 的基本结构：包含头文件、main 函数入口、标准库函数调用。C 的设计哲学是"信任程序员"——它不会像 Java 那样强制检查很多东西。

</details>
