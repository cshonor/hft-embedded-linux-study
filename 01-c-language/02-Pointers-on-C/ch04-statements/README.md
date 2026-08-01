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
