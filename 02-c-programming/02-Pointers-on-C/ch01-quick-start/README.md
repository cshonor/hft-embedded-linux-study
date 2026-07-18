# 第 1 章 快速上手

**Quick Start**

## 本章讲什么

全书**铺垫章**：最小可运行 C 程序、**WSL/Linux 编译四阶段**、运行与 **gdb**、极简 I/O 与 **`&` 取地址**。不深入语法，只回答：**一段 C 代码从编写到 CPU 执行的完整链路** —— 后续 DPDK、内核、交易程序构建流程的缩影。

## 学习重点

- 最简骨架：`#include`、`int main(void)`、`printf`、`return 0`
- **预处理 → 编译 → 汇编 → 链接**；动态 vs **`-static`**
- **`scanf(..., &val)`** 与返回值校验；**stdio vs write** 延迟
- 栈上大数组风险；**gdb** 断点与段错误定位
- 书中示例：**read_column_numbers** + **rearrange** 串联 I/O 与控制流

## 场景价值

| 方向 | 本章收获 |
|------|----------|
| DPDK / 内核 | 四阶段 = 所有底层构建本质 |
| HFT | 静态链接、绕过 stdio 缓冲的意识 |
| 嵌入式 | 栈限制、退出码、Makefile 模板 |

## 前后章节

| 方向 | 章节 |
|------|------|
| 前序 | 无（全书开篇） |
| 后序 | ch02 基本概念；ch03 数据；ch06 指针；ch15 I/O；ch18 运行时 |
| 配套 | 《C陷阱与缺陷》ch01/ch02 规避词法新手 bug |

## 实操（建议完成）

1. `-E/-S/-c` 查看各阶段产物  
2. 动态 vs 静态二进制大小  
3. gdb 断点 + `print &变量`  
4. 故意省略 `&` 用 gdb 定位  
5. 循环读入 + scanf 返回值容错  

## 小节

- [1.1 简介](./1.1-introduction/1.1-introduction.md)
  - [1.1.1 空白和注释](./1.1-introduction/1.1.1-空白和注释.md)
  - [1.1.2 预处理指令](./1.1-introduction/1.1.2-预处理指令.md)
  - [1.1.3 main 函数](./1.1-introduction/1.1.3-main函数.md)
  - [1.1.4 read_column_numbers 函数](./1.1-introduction/1.1.4-read_column_numbers函数.md)
  - [1.1.5 rearrange 函数](./1.1-introduction/1.1.5-rearrange函数.md)
- [1.2 补充说明](./1.2-补充说明.md)
- [1.3 编译](./1.3-编译.md)
