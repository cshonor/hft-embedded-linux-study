# ch04 Demo

## 四阶段编译

```bash
make demo01
readelf -S demo01.o
readelf -S demo01
```

## 静态库 / 动态库

```bash
make demo_static demo_shared
./demo_static
./demo_shared
nm libdemo.a
ldd demo_shared
```

## 工具

```bash
readelf -h demo01
objdump -h demo01
nm demo01.o
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
