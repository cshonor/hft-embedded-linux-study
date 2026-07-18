# ch06 Demo

```bash
make all

./demo01_memory_layout/main
./demo02_static_local/main
./demo03_stack_frame/main
./demo04_buffer_overflow/main
```

## demo04 栈溢出原理

默认编译可能因 `-fstack-protector` 在 `memset` 越界时直接 abort。教学对比：

```bash
make -C demo04_buffer_overflow main_noprotect
./demo04_buffer_overflow/main_noprotect
```

观察 `auth` 从 0 被改写——说明栈上局部数组越界可破坏相邻数据；严重时可覆盖 **返回地址 RA**。

## 工具

```bash
size demo01_memory_layout/main
readelf -S demo01_memory_layout/main
nm demo01_memory_layout/main | grep -E 'global|static'
```
