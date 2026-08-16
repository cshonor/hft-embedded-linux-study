# ch06 Demo

```bash
make all

# packed / aligned
./demo01_packed_struct
gdb -batch -ex 'break main' -ex run -ex 'x/16xb &p' ./demo01_packed_struct

# 自定义 ELF 段
readelf -S demo02_custom_section | grep -E 'my_|Name'
nm demo02_custom_section | grep g_custom

# weak 弱符号
./demo03_weak              # 输出 weak 默认
./demo03_weak_override     # 输出 strong 覆盖

# constructor / destructor
./demo04_constructor

# 柔性数组
./demo05_flexible_array

# 日志宏 + 语句表达式 MAX
./demo06_log_macro

# 内嵌汇编 / 寄存器约束
./demo07_reg_asm
```

## 工具验证

```bash
size demo01_packed_struct demo02_custom_section
objdump -t demo02_custom_section | grep my_
gcc -Wall -Wformat -o fmt_test demo06_log_macro.c   # format 属性见 6.8
```

## demo03 说明

| 目标 | 链接 | 行为 |
|------|------|------|
| `demo03_weak` | main + hw_weak.c | 调用 weak 默认 stub |
| `demo03_weak_override` | main + weak + hw_strong.c | 强符号覆盖 weak |

## demo07 ARM 交叉（可选）

```bash
# 见 ch03 交叉工具链
# register int val asm("r0") = 1;
# asm volatile("mov %0, #0" : "=r"(val));
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
