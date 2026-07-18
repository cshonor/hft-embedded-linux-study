# ch09 Demo

```bash
make all
./demo01_2d_pass/main
./demo02_string_arrays/main
./demo03_flex_vs_ptr/main
./demo04_ptr_arith_2d/main
```

## demo03 柔性数组 vs 指针成员

- **Flex**：`malloc(sizeof(MsgFlex)+cap)` 一次分配，header 与 buf 连续
- **Ptr**：header + `malloc(cap)` 两次分配，地址分离

## demo04 二维指针步长

`int (*p)[4]; p+1` 跳过 **16 字节**（4×sizeof(int)）；`&arr+1` 跳过整个 `int[3][4]`。
