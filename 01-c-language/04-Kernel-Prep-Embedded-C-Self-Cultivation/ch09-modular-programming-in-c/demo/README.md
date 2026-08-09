# ch09 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/04-Kernel-Prep-Embedded-C-Self-Cultivation/ch09-modular-programming-in-c/demo

make all
./demo01_minimal/demo01_minimal
./demo02_make/demo02_app
make mod_uart          # 仅重编 uart 静态库
make demo03            # CMake 构建
./demo03_cmake/build/demo03_cmake
./demo04_weak/demo04_weak
./demo04_weak/demo04_weak_board
./demo05_log_err/demo05_log_err
./demo06_callback/demo06_callback
```

## 目录结构

```
demo/
  common/          err.h log.h err.c libcommon.a
  demo01_minimal/  app/ driver/ utils/  — 最小三层
  demo02_make/     分层 Makefile + mod_uart + install
  demo03_cmake/    CMake add_library 链接
  demo04_weak/     platform weak + hw 强符号覆盖
  demo05_log_err/  统一日志与错误码
  demo06_callback/ sensor 回调解耦循环依赖
```

## demo02 单模块编译

```bash
make -C demo02_make mod_uart
make -C demo02_make mod_utils
make -C demo02_make install   # 输出到 demo/out/
```

## demo03 CMake

```bash
mkdir -p demo03_cmake/build && cd demo03_cmake/build
cmake ..
make
```

## .gitignore 建议

```
*.o *.a *.so demo*/demo*
build/
out/
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
