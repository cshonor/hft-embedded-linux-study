# ch08 Demo

```bash
make all

./demo01_encapsulation
./demo02_inherit
./demo03_polymorphism
valgrind --leak-check=full ./demo04_lifecycle
./demo05_multi_iface
./demo06_layered
```

## demo03 多态 UART/SPI

`dev_ops.c` 定义 `uart_ops` / `spi_ops` 虚表；`app_send()` 仅依赖 `struct dev_obj *`，运行时绑定不同硬件实现。

```bash
gdb ./demo02_inherit
(gdb) print dev->open
(gdb) print &uart
```

## demo06 三层目录

```
layered/
  abstract/   device.h device.c  — 抽象接口
  hw/         uart.c              — 硬件适配
  app/        app.c main.c        — 业务层
```

业务层只 `#include` 抽象头文件，不直接依赖 `hw/uart.c` 内部实现。

## valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all ./demo03_polymorphism
valgrind --leak-check=full ./demo04_lifecycle
```

---

## 代码自测

**题目 1：** 本章 demo 目录的实践代码涉及哪些知识点？如何编译运行？
```c
// 典型 demo 编译流程
cd demo/
make all    # 编译
./app       # 运行
make clean  # 清理
```
<details>
<summary>参考答案</summary>

嵌入式 C 自我修养这本书是标准 C 到内核开发的桥梁。内核代码大量使用 __attribute__/typeof/container_of/section/weak 等 GNU C 扩展，侵入式链表和函数指针实现的 OOP，以及头文件/源文件分离的模块化设计——这些标准 C 教材不讲，本书系统讲解。

学习路线：ch01 工具链 -> ch02 计算机架构 -> ch03 ARM 汇编 -> ch04 编译链接 -> ch05 内存栈管理 -> ch06 GNU C 扩展 -> ch07 数据存储与指针 -> ch08 OOP -> ch09 模块化编程 -> ch10 多任务与 OS。

</details>
