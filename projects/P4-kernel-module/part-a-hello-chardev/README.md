# Part A — hello chardev (kernel module stub)

Builds only on Linux with kernel headers installed.

```bash
make
# sudo insmod hello.ko
# sudo rmmod hello
# dmesg | tail
```

Userspace smoke (no module needed to compile):

```bash
make -C userspace
./userspace/hello_user
```
