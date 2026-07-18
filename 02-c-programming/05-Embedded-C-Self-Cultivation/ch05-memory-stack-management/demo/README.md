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
