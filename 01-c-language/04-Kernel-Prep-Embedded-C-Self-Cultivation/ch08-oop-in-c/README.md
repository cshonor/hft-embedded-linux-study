# 第 8 章 C 语言的面向对象编程思想

**OOP Patterns in C for Kernel, Driver & DPDK**

## 本章目标

建立 **struct + 函数指针 + ops 表** 模拟 OOP 的完整图景：封装（不透明类型）、单继承（首成员嵌入）、多态（DevOps/vtable）、多接口（mixin ops）。读懂 Linux **`list_head`**、**device/bus**、**file_operations`** 与 DPDK **`eth_dev_ops`**；能设计 **abstract/hw/app** 三层驱动并用 **weak 符号**（**ch06**）做可替换 BSP。衔接 **ch07 函数指针**、**ch09 模块化**、**ch10 并发**。

## 前置依赖

| 章节 | 内容 |
|------|------|
| **[ch05](../ch05-memory-stack-management/)** | 堆/栈、`malloc`/`free`、对象生命周期 |
| **[ch06](../ch06-gnu-c-extensions/)** | `container_of`、`weak`/`alias`、指定初始化 |
| **[ch07](../ch07-data-storage-and-pointers/)** | 结构体指针、`typedef`、**7.12 函数指针** |

## 环境

- **编译器**：GCC 或 Clang，**`-std=gnu11 -Wall -Wextra`**
- **工具**：`make`、`nm`（查看 ops 符号）、`readelf`（复习 weak/strong）
- **头文件**：`<stdlib.h>`、`<stdint.h>`
- **拓展**：**demo/**（见下）

## 快速操作 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Kernel-Prep-Embedded-C-Self-Cultivation/ch08-oop-in-c/demo

make all

# 不透明 struct + create/destroy/get/set
./demo01_encapsulation

# 首成员继承：UartDev/SPI 内嵌 Device
./demo02_inherit

# DevOps 运行时多态：uart_ops / spi_ops
./demo03_polymorphism
valgrind --leak-check=full ./demo04_lifecycle

# PowerOps + CommOps 多接口
./demo05_multi_iface

# abstract/hw/app 三层
./demo06_layered

make clean
```

## 七大知识模块

| 模块 | 目录 | 核心 |
|------|------|------|
| **1 分层复用** | **8.1** | abstract/hw/app、weak stub、依赖方向 |
| **2 OOP 概念** | **8.2**、**8.2.1–8.2.4** | 封装/继承/多态、类映射、vtable |
| **3 封装实践** | **8.3**、**8.3.1** | 不透明类型、create/destroy API |
| **4 内核容器** | **8.3.2** | `list_head`、`container_of`、侵入式链表 |
| **5 设备模型** | **8.3.3–8.3.4** | `struct device`、`bus_type`、match/probe |
| **6 继承扩展** | **8.4**、**8.4.1–8.4.3** | 首成员、PIMPL、抽象类、多 ops 接口 |
| **7 多态落地** | **8.5** | `file_operations`、`eth_dev_ops`、VFS/DPDK |

## Demo 清单

| Demo | 内容 | 对应小节 |
|------|------|----------|
| **demo01** | 不透明 `counter_t`、create/destroy/inc | **8.2.2**、**8.3.1** |
| **demo02** | `Device` 基类 + `UartDev`/`SpiDev` 首成员 | **8.2.3**、**8.4** |
| **demo03** | `DevOps` 表、`uart_ops`/`spi_ops` 分派 | **8.2.4**、**8.5** |
| **demo04** | 静态/堆实例、生命周期配对 | **8.2.2**、**ch05** |
| **demo05** | `CommOps` + `PowerOps` 多接口 mixin | **8.4.3** |
| **demo06** | abstract/hw/app 目录、weak 覆盖 | **8.1**、**ch06 6.9** |

## 考核要点

1. 用表格说明 **C 与 OOP 四类概念**（类/对象/继承/多态）的对应实现（**8.2.1**）
2. 写出**不透明类型**封装套路：`typedef` + 仅 `.c` 可见的 struct 定义（**8.3.1**、**demo01**）
3. 解释**首成员继承**为何能 `(Device *)&uart_dev` 向上转型，并写出 `container_of` 向下转型（**8.2.3**、**demo02**）
4. 设计 **`struct DevOps`** 并实现 `device_send()` 统一分派，含 NULL 检查（**8.2.4**、**demo03**）
5. 对比**静态对象**与**堆对象**在生命周期、失败模式上的差异（**8.2.2**、**demo04**）
6. 说明 **weak 符号** 在 abstract/hw 分层中如何替代 `#ifdef`（**8.1**、**demo06**、**ch06 6.9**）
7. 描述 **`list_head` 侵入式链表**的 embed、注册、遍历三步（**8.3.2**）
8. 画出 **device_register → bus match → probe** 时序（**8.3.3–8.3.4**）
9. 区分**单继承（首成员）**与**多接口（多 ops 指针）**的适用场景（**8.4.3**、**demo05**）
10. 列举 **`file_operations`** 与 **DPDK `eth_dev_ops`** 中各一个多态入口函数（**8.5**）

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | **ch05** 堆与生命周期；**ch06** `container_of`/weak；**ch07** 函数指针 |
| 后置 | **ch09** 头文件与模块化；**ch10** OS/驱动并发模型 |

## 小节

- [8.1 代码复用与分层思想](./8.1-代码复用与分层思想.md)
- [8.2 面向对象编程基础](./8.2-oop-basics/8.2-面向对象编程基础.md)
  - [8.2.1 什么是OOP](./8.2-oop-basics/8.2.1-什么是OOP.md)
  - [8.2.2 类的封装与实例化](./8.2-oop-basics/8.2.2-类的封装与实例化.md)
  - [8.2.3 继承与多态](./8.2-oop-basics/8.2.3-继承与多态.md)
  - [8.2.4 虚函数与纯虚函数](./8.2-oop-basics/8.2.4-虚函数与纯虚函数.md)
- [8.3 Linux内核中的OOP思想：封装](./8.3-encapsulation/8.3-Linux内核中的OOP思想-封装.md)
  - [8.3.1 类的C语言模拟实现](./8.3-encapsulation/8.3.1-类的C语言模拟实现.md)
  - [8.3.2 链表的抽象与封装](./8.3-encapsulation/8.3.2-链表的抽象与封装.md)
  - [8.3.3 设备管理模型](./8.3-encapsulation/8.3.3-设备管理模型.md)
  - [8.3.4 总线设备模型](./8.3-encapsulation/8.3.4-总线设备模型.md)
- [8.4 Linux内核中的OOP思想：继承](./8.4-inheritance/8.4-Linux内核中的OOP思想-继承.md)
  - [8.4.1 继承与私有指针](./8.4-inheritance/8.4.1-继承与私有指针.md)
  - [8.4.2 继承与抽象类](./8.4-inheritance/8.4.2-继承与抽象类.md)
  - [8.4.3 继承与接口](./8.4-inheritance/8.4.3-继承与接口.md)
- [8.5 Linux内核中的OOP思想：多态](./8.5-Linux内核中的OOP思想-多态.md)


---

## 章节自测

> C 没有 class 关键字，但内核到处是 OOP。看代码 → 想答案 → 点开验证。

### Q1: 不透明类型封装

```c
// sensor.h
typedef struct Sensor Sensor;  // 前向声明，不暴露内部
Sensor *sensor_create(int type);
int    sensor_read(Sensor *s);
void   sensor_destroy(Sensor *s);

// sensor.c
struct Sensor {               // 完整定义只在此
    int type;
    int value;
    void (*calibrate)(Sensor *);
};
```

> 为什么 `.h` 不暴露 `struct Sensor` 的定义？

<details>
<summary>答案与复习指引</summary>

**答案：** **不透明指针**模式——调用者只持有 `Sensor*`，不能直接访问 `type`/`value` 等成员。好处：
1. **封装** — 强制走 API 函数，防止外部直接修改内部状态
2. **ABI 稳定** — 改 `struct Sensor` 内部布局不影响使用者重新编译
3. **隐藏实现** — 源码保护

**内核实例：** `struct file`、`struct inode`、`struct task_struct` 等。

**复习：** → [8.3 封装与私有指针](./8.3-encapsulation/8.3-封装与私有指针.md)

</details>

### Q2: 首成员嵌入实现继承

```c
struct Device {
    char name[32];
    int  status;
};

struct UartDevice {
    struct Device base;   // 首成员嵌入 → "继承"
    int baud_rate;
    int port;
};

// 为什么可以这样做？
struct UartDevice uart = {0};
struct Device *dev = (struct Device *)&uart;
// dev->name 合法吗？
```

<details>
<summary>答案与复习指引</summary>

**答案：** 合法。C 标准保证结构体首成员的地址 = 结构体本身的地址。所以 `&uart` = `&uart.base`，`dev` 指向 `uart.base`，访问 `dev->name` 和 `dev->status` 是安全的。

**内核实例：** `struct usb_device` 首成员是 `struct device`，可以安全转成 `struct device*` 交给通用设备框架。

**这就是 C 的"继承"——首成员嵌入。** 但只能"单继承"（C 不支持多首成员）。

**复习：** → [8.4 继承与私有指针](./8.4-inheritance/8.4.1-继承与私有指针.md)

</details>

### Q3: file_operations 多态

```c
struct file_operations {
    int (*open)(struct file *);
    ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
    int (*release)(struct file *);
};

// 不同驱动注册不同的 ops
struct file_operations uart_fops = {
    .open = uart_open,
    .read = uart_read,
    .write = uart_write,
};
struct file_operations net_fops = {
    .open = net_open,
    .read = net_read,
};
```

> 这是什么设计模式？VFS 怎么知道调哪个驱动的函数？

<details>
<summary>答案与复习指引</summary>

**答案：** **策略模式 / 多态**——通过函数指针表实现运行时多态。VFS 层的 `filp->f_op->read(filp, ...)` 根据 `file->f_op` 指向的不同 `file_operations` 表，调用不同驱动的 `read` 实现。

**运行时绑定：** `open("/dev/uart0")` → 内核找到 `uart0` 对应的 `inode` → 取其 `file_operations` → 赋给 `file->f_op` → 后续 `read()` 调用 `f_op->read` = `uart_read`。

**DPDK 类似：** `rte_eth_dev` 的 `dev_ops` 表。

**复习：** → [8.5 Linux内核中的OOP思想：多态](./8.5-Linux内核中的OOP思想-多态.md)
