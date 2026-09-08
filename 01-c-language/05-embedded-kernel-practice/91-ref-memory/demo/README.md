# ch05 Demo

```bash
make all
size demo01_memory_zone
./demo01_memory_zone

gdb ./demo02_stack_frame
# (gdb) break recurse
# (gdb) run
# (gdb) bt
# (gdb) x/32xb $sp

valgrind --leak-check=full ./demo04_heap_leak
./demo05_static
```

## demo03 栈溢出（慎用，仅 Linux/WSL）

两种模式：

| 命令 | 现象 |
|------|------|
| `./demo03_stack_overflow` | 小缓冲区写越界，覆盖 saved LR |
| `./demo03_stack_overflow r` | 递归 + 每层 1KB 局部数组，耗尽栈 |

### 编译

```bash
# 无 canary：观察原始 SIGSEGV / 跑飞
make demo03_stack_overflow CFLAGS="-g -O0 -Wall -fno-stack-protector"

# 有 canary：观察 *** stack smashing detected ***
make demo03_stack_overflow CFLAGS="-g -O0 -Wall -fstack-protector-all"
```

### GDB 调试步骤（模式 A：缓冲区越界）

```bash
ulimit -c unlimited
gdb -q ./demo03_stack_overflow
(gdb) break smash_frame
(gdb) run
(gdb) info registers sp fp lr pc
(gdb) x/64xb $sp          # 越界写入后查看栈原始字节
(gdb) continue            # 返回时触发 SIGSEGV 或 abort
(gdb) bt
(gdb) frame 0
(gdb) info locals
```

要点：`memset` 写满 512 字节会覆盖 `buf[32]` 之上的 saved frame pointer / return address；无 protector 时 `continue` 后 `bt` 可能乱序或 SIGSEGV。

### GDB 调试步骤（模式 B：递归耗尽栈）

```bash
ulimit -s 4096            # 限制栈 4MB → 更易复现
make demo03_stack_overflow CFLAGS="-g -O0 -Wall"
gdb -q ./demo03_stack_overflow
(gdb) run r
(gdb) bt                  # 极深调用链
(gdb) info registers sp lr
```

### ASAN 对比（可选）

```bash
gcc -g -O0 -fsanitize=address -o demo03_asan demo03_stack_overflow.c
./demo03_asan             # 报告 stack-buffer-overflow 精确行号
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
