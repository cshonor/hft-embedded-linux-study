# Part A — hello 字符设备

默认 `make` / `make test` 只编用户态探测程序（WSL 常常没有可用 `KDIR`）。

有内核头时：

```bash
make modules
# sudo insmod hello.ko
# sudo rmmod hello
# dmesg | tail
```

```bash
make test
```
