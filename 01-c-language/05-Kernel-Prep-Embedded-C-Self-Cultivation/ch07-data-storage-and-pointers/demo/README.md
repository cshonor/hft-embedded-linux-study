# ch07 Demo

```bash
make all

./demo01_array_ptr
gdb -batch -ex run -ex 'print sizeof(buf)' -ex 'print sizeof(p)' -ex 'x/16xb buf' ./demo01_array_ptr

./demo02_double_ptr
./demo03_struct_offset
./demo04_func_ptr

# volatile 汇编对比（建议 -O2 更明显）
make demo05_volatile CFLAGS="-g -O2 -Wall -std=gnu11"
objdump -d demo05_volatile | grep -A8 poll_volatile
objdump -d demo05_volatile | grep -A8 poll_normal

./demo06_dangling
# 取消 demo06 内注释后: gdb ./demo06_dangling

./demo07_mmio
```

## demo04 函数指针跳转表

四则运算 `jump_table[OP_*]`，衔接 **7.12** 与 **ch08 OOP**。

## demo05 说明

| 变量 | 行为 |
|------|------|
| `volatile int irq_flag` | 循环内每次从内存 reload |
| 普通 `int` | `-O2` 可能被优化成死循环或常量 |

```bash
gcc -S -O2 -o v.s demo05_volatile.c
```

## demo07 MMIO

用户态 `fake_uart` 模拟 `volatile` 结构体寄存器映射；真板子替换为固定物理地址指针。

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
