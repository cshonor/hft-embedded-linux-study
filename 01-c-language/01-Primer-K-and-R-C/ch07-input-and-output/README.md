# 第 7 章 输入与输出

**Input and Output**

## 本章讲什么

面向**终端、文件、字符流**的标准 I/O：字符读写、格式化 `printf`/`scanf`、**`FILE*` 流**、二进制 **`fread`/`fwrite`**、定位与错误处理。程序与外部数据交互的基础；承接第 6 章 struct，为第 8 章 UNIX 系统接口铺垫。

## 学习重点

- **字符流统一模型**；**`EOF`** vs 错误（`feof`/`ferror`）
- **文本 vs 二进制**（`b` 模式、换行转换坑）
- **`snprintf`/`fgets`** 替代不安全 `sprintf`/`gets`
- **`fread`/`fwrite` + struct** 与 **padding/endian** 不可移植
- **`stderr`** 与 **`fseek`** 随机访问

## 场景映射

| 方向 | 本章技能 |
|------|----------|
| OS / UEFI | 镜像读写、GOP 类 printf、stdout/stderr 调试 |
| HFT | tick 落盘/回放、snprintf 日志、sscanf 轻量解析 |
| 嵌入式 Linux | 配置/传感器文件、串口字符流 |

## 难点

文本/二进制模式、缓冲区溢出、`fseek` 大文件、struct 整块读写跨平台。

## 小节

- [7.1 标准输入输出](./7.1-标准输入输出.md)
- [7.2 格式化输出 printf 函数](./7.2-格式化输出printf函数.md)
- [7.3 变长参数表](./7.3-变长参数表.md)
- [7.4 格式化输入 scanf 函数](./7.4-格式化输入scanf函数.md)
- [7.5 文件访问](./7.5-文件访问.md)
- [7.6 错误处理 stderr 和 exit](./7.6-错误处理stderr和exit.md)
- [7.7 行输入和行输出](./7.7-行输入和行输出.md)
- [7.8 其它函数](./7.8-other-functions/7.8-其它函数.md)
  - [7.8.1 字符串操作函数](./7.8-other-functions/7.8.1-字符串操作函数.md)
  - [7.8.2 字符类别测试和转换函数](./7.8-other-functions/7.8.2-字符类别测试和转换函数.md)
  - [7.8.3 ungetc 函数](./7.8-other-functions/7.8.3-ungetc函数.md)
  - [7.8.4 命令执行函数](./7.8-other-functions/7.8.4-命令执行函数.md)
  - [7.8.5 存储管理函数](./7.8-other-functions/7.8.5-存储管理函数.md)
  - [7.8.6 数学函数](./7.8-other-functions/7.8.6-数学函数.md)
  - [7.8.7 随机数发生器函数](./7.8-other-functions/7.8.7-随机数发生器函数.md)
