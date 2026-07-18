# ch03 Demo

## demo03：C 调用 ARM 汇编

```bash
sudo apt install gcc-arm-linux-gnueabihf qemu-user-static
source ../../devenv/buildenv.sh
export CROSS_COMPILE=arm-linux-gnueabihf-
make all
qemu-arm ./demo03
arm-linux-gnueabihf-objdump -d demo03   # 查看 add_asm
```

## demo02 思路（本机 x86）

```bash
gcc -g -O0 -c demo03_main.c -o main.o   # 仅 C 部分
gcc -g -O0 -S demo03_main.c            # 生成 .s 对照
objdump -dS a.out
```
