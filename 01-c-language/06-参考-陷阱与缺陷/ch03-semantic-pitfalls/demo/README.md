# ch03 Demo

```bash
make all
./demo01_array_ptr/main
./demo02_signed_unsigned/main
./demo03_unsigned_loop/main
./demo04_arg_order/main
./demo05_bounds/main
./demo06_ro_string/main
```

`demo05` 需 `-fno-stack-protector` 时更易观察 guard 被改写（Makefile 已设）。
