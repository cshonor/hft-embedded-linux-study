# 第 15 章 输入/输出函数

**Input/Output Functions**

## 本章讲什么

**`<stdio.h>`**：**FILE 流**、fopen/fclose、字符/行/格式化 IO、**fread/fwrite 二进制**、fseek/fflush、ferror。日志落盘与行情持久化刚需；数据面需知 stdio 缓冲延迟代价。

## 学习重点

- **stdin/stdout/stderr**；stderr **无缓冲**
- fopen **`rb`/`wb`**；fclose 防句柄泄漏
- **fgets** 读配置；禁用 sprintf/gets
- **snprintf** + **fprintf** 日志；**fflush** 防崩溃丢日志
- **fread/fwrite** 读写 packed struct
- **fgetc** 返回值用 **int**
- **feof 不能作循环前置**；用返回值 + **ferror**
- 热路径：**stdio vs open/read/write**

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| HFT | 行情持久化、滚动日志、snprintf |
| DPDK | pcap 二进制、统计 fprintf |
| 用户态 | 设备/管道 fopen；理解缓冲层 |

## 线上陷阱（汇总）

1. 二进制误用文本 IO  
2. sprintf 溢出  
3. 忘记 fclose  
4. char 存 fgetc 返回值  
5. feof 前置循环  
6. stdout 行缓冲延迟  
7. 频繁 fopen 碎片  

## 实操（建议完成）

1. rb + fread/fwrite Quote  
2. sprintf vs snprintf 溢出  
3. fprintf + fflush(stderr)  
4. fseek 按 seq 跳转  
5. fread 循环 + ferror  
6. stdout 无 `\n` vs fflush  
7. DPDK 风格日志封装  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch07 stdarg；ch09/ch10 二进制 struct |
| 后序 | ch16 标准库；K&R UNIX IO |
| 配套 | 《C陷阱与缺陷》ch05 |

## 小节

- [15.1 错误报告](./15.1-错误报告.md)
- [15.2 终止执行](./15.2-终止执行.md)
- [15.3 标准 I/O 函数库](./15.3-标准I-O函数库.md)
- [15.4 ANSI I/O 概念](./15.4-ansi-i-o-concepts/15.4-ansi-i-o-concepts.md)
- [15.5 流 I/O 总览](./15.5-流I-O总览.md)
- [15.6 打开流](./15.6-打开流.md)
- [15.7 关闭流](./15.7-关闭流.md)
- [15.8 字符 I/O](./15.8-字符I-O.md)
- [15.9 未格式化的行 I/O](./15.9-未格式化的行I-O.md)
- [15.10 格式化的行 I/O](./15.10-格式化的行I-O.md)
- [15.11 二进制 I/O](./15.11-二进制I-O.md)
- [15.12 刷新和定位](./15.12-刷新和定位.md)
- [15.13 改变缓冲方式](./15.13-改变缓冲方式.md)
- [15.14 流错误函数](./15.14-流错误函数.md)
- [15.15 临时文件](./15.15-临时文件.md)
- [15.16 文件操纵函数](./15.16-文件操纵函数.md)
