# ch03 Demo

## demo03：C 调用 ARM 汇编

```bash
sudo apt install gcc-arm-linux-gnueabihf qemu-user-static
source ../../devenv/buildenv.sh
export CROSS_COMPILE=arm-linux-gnueabihf-
make all
qemu-arm ./demo03
arm-linux-gnueabihf-objdump -d demo03   # 查看 add_asm
```

## demo02 思路（本机 x86）

```bash
gcc -g -O0 -c demo03_main.c -o main.o   # 仅 C 部分
gcc -g -O0 -S demo03_main.c            # 生成 .s 对照
objdump -dS a.out
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
