# 第 13 章 高级指针话题

**Advanced Pointer Topics**

## 本章讲什么

指针高阶：**多级指针**、**函数指针**、**转移表**、复杂声明解读、**回调 + void \***、argv。DPDK 驱动表、内核 file_operations、HFT 报文分发的分水岭章节。

## 学习重点

- **`char **`** 动态串数组；≤ 三级指针
- **typedef** 封装函数指针
- **函数指针数组** 分发 vs if-else
- 回调：**判空** + **void \*priv**
- 声明解读：指针数组 / 数组指针 / 函数指针 / 函数指针数组
- **argv** = `char **` 实例
- 字面量 **`const char *`**

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | PMD ops、报文 dispatch、mempool 钩子 |
| 内核 | file_operations、private_data |
| HFT | 表驱动分发、策略回调、通用 destroy |

## 线上陷阱（汇总）

1. 函数指针数组声明写错  
2. 回调未判空调用  
3. void* 强转类型不匹配  
4. 多级指针逐层 free 遗漏  
5. 不用 typedef 难维护  
6. 不同签名函数赋给同一函数指针  

## 实操（建议完成）

1. typedef + 报文 dispatch 表  
2. 仿 file_operations 挂载 read/write  
3. 5 组复杂声明拆解  
4. event_mgr 注册/触发  
5. 空调回调段错误  
6. pool_destroy + free_cb  
7. if-else vs 转移表对比  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch06–ch07；ch12 二级指针 |
| 后序 | ch14 宏；ch17 ADT；ch18 ABI |
| 配套 | 《C陷阱与缺陷》ch03、ch04 |

## 小节

- [13.1 进一步探讨指向指针的指针](./13.1-进一步探讨指向指针的指针.md)
- [13.2 高级声明](./13.2-高级声明.md)
- [13.3 函数指针](./13.3-function-pointers/13.3-function-pointers.md)
- [13.4 命令行参数](./13.4-命令行参数.md)
- [13.5 字符串常量](./13.5-字符串常量.md)
