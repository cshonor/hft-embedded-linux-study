# 第 2 章 基本概念

**Basic C Concepts**

## 本章讲什么

全书**语法规则地基**：Token 词法、注释与空白、**声明/定义**、**作用域**、**存储类**、程序**翻译与执行**模型。承接 ch01 骨架，为指针、结构体、内存布局建立标准化认知。

## 学习重点

- **五类 Token** + **最长匹配**（`a+++b`）
- **声明 vs 定义** → 链接 `multiple definition` / `undefined reference`
- **四类作用域** + **遮蔽**
- **static** 两义（局部生命周期 / 文件内链接）、**extern**
- ELF **.text / .data / .bss / stack**；bss 清零 vs 栈脏数据
- **if 分号**、工程命名与 **2.3 风格**

## 场景价值（内核 / DPDK / HFT）

| 价值 | 说明 |
|------|------|
| 链接理论 | extern/static 解 90% 多文件报错 |
| 内存生命周期 | static 池、单次硬件 init |
| 符号隔离 | static 私有函数 |
| 防御 bug | 遮蔽、词法、分号陷阱 |

## 实操（建议完成）

1. 多文件 static vs 全局，观察链接器  
2. static 局部只初始化一次  
3. 变量遮蔽实验  
4. if 分号错误  
5. `readelf -S ./a.out`  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch01-quick-start |
| 后序 | ch03 数据；ch04 语句；ch06 指针；ch18 运行时 |
| 配套 | 《C陷阱与缺陷》ch01 词法、ch02 语法、ch04 链接 |

## 小节

- [2.1 环境](./2.1-the-environment/2.1-the-environment.md)
  - [2.1.1 翻译](./2.1-the-environment/2.1.1-翻译.md)
  - [2.1.2 执行](./2.1-the-environment/2.1.2-执行.md)
- [2.2 词法规则](./2.2-lexical-rules/2.2-lexical-rules.md)
  - [2.2.1 字符](./2.2-lexical-rules/2.2.1-字符.md)
  - [2.2.2 注释](./2.2-lexical-rules/2.2.2-注释.md)
  - [2.2.3 自由形式的源代码](./2.2-lexical-rules/2.2.3-自由形式的源代码.md)
  - [2.2.4 标识符](./2.2-lexical-rules/2.2.4-标识符.md)
  - [2.2.5 程序的形式](./2.2-lexical-rules/2.2.5-程序的形式.md)
- [2.3 程序风格](./2.3-程序风格.md)
