# ch07 Demo

```bash
make all

./demo01_struct_align/main          # sizeof / offsetof / 对齐手算
./demo02_stack_dangle/main          # 返回栈局部数组 → 悬挂指针
./demo03_const_rodata/main          # const 三种形式
./demo04_false_sharing/main         # 伪共享 vs 64B 填充（需多核更明显）
```

## demo01 结构体对齐分步模板

用 `offsetof` 打印各成员偏移，对照 README/7.2 手算：`a@0 → 填 3B → b@4 → c@8 → 尾填至 8 对齐`。

## demo04 伪共享

两线程分别递增相邻 `int64_t` 时，共享缓存行导致无效化风暴；`char pad[56]` 隔离后通常更快（ITERS 可调小做快速试跑）。

```bash
# 快速试跑（改 main.c 中 ITERS 为 5000000）
make -C demo04_false_sharing && ./demo04_false_sharing/main
```

## 工具

```bash
size demo01_struct_align/main
readelf -S demo03_const_rodata/main | grep rodata
valgrind --leak-check=full ./demo02_stack_dangle/main
```
