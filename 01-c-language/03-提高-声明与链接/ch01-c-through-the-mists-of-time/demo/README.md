# ch01 Demo

```bash
make all
./demo01_kr_vs_ansi
./demo02_macro_trap
```

## demo01

对比 ANSI 函数原型与 K&R 旧式声明；用 `-std=c89` 编译，尝试传入错误类型观察编译器报错。

## demo02

宏纯文本替换：`SQUARE(a+1)` vs 未加括号的 `BAD_SQUARE(a+1)`。
