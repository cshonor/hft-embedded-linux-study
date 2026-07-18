# 第 16 章 标准函数库

**Standard Library**

## 本章讲什么

C 标准库工具集总览：**stdlib**、**string**、**ctype**、**math**、**time**、**stdarg**、信号与执行环境。区分标准库封装 vs 系统调用；规避隐藏 UB 与 HFT 性能损耗。

## 学习重点

### 按模块（与全书衔接）

| 模块 | 头文件 | 要点 |
|------|--------|------|
| 内存/串 | string.h | **mem\*** 二进制；**str\*** 文本（ch09） |
| 堆 | stdlib.h | malloc/posix_memalign（ch11） |
| 转换 | stdlib.h | **strtoll** 非 atoi |
| 随机 | stdlib.h | rand 线程不安全 → rand_r/rte_rand |
| 排序 | stdlib.h | qsort/bsearch + 回调（ch13） |
| 控制 | stdlib.h | exit/atexit；**禁 system** |
| 字符 | ctype.h | unsigned char（ch09） |
| 数学 | math.h | 价用整型，浮点仅统计 |
| 时间 | time.h | clock_gettime 纳秒延迟 |
| 可变参 | stdarg.h | vfprintf 日志（ch15） |

- **二进制 → mem\***；**文本 → str\***
- DPDK：**rte_memcpy**、**rte_rand**、自研排序对标本章优化点

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | 标准库设计参照、规避锁/碎片 |
| 网关 | strtoll 解析、qsort 合约表、atexit 落盘 |
| HFT | 热路径禁 malloc/system/qsort 通用路径 |

## 线上陷阱（汇总）

1. atoi 静默失败  
2. rand 多线程竞争  
3. ctype 负 char UB  
4. qsort 回调强转错  
5. system 子进程抖动  
6. memcpy 重叠  
7. 热路径 malloc 锁  

## 实操（建议完成）

1. atoi vs strtoll  
2. qsort + bsearch 订单 ID  
3. memcpy vs memmove 重叠  
4. 多线程 rand vs rand_r  
5. clock 测解析耗时  
6. atexit 快照落盘  
7. ctype 负 char UB  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch09/ch11/ch13/ch15 |
| 后序 | ch17 ADT；ch18 syscall |
| 配套 | 《C陷阱与缺陷》ch05 |

## 小节

- [16.1 整型函数](./16.1-整型函数.md)
- [16.2 浮点型函数](./16.2-浮点型函数.md)
- [16.3 日期和时间函数](./16.3-日期和时间函数.md)
- [16.4 非本地跳转](./16.4-非本地跳转.md)
- [16.5 信号](./16.5-信号.md)
- [16.6 打印可变参数列表](./16.6-打印可变参数列表.md)
- [16.7 执行环境](./16.7-执行环境.md)
- [16.8 locale](./16.8-locale.md)
