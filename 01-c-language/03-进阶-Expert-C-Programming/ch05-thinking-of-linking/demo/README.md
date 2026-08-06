# ch05 Demo

```bash
make all

# 四阶段 + nm/readelf -r
make -C demo01_four_stage inspect
./demo01_four_stage/main

# 静态库 ar rcs
./demo02_static_lib/demo_static
nm demo02_static_lib/libadd.a

# 弱符号 + 强符号
./demo03_weak_strong/demo_weak

# 链接脚本 kernel.lds
make -C demo04_linker_script
readelf -S demo04_linker_script/kernel.elf
```

## demo04 kernel.lds

导出 `_stext/_etext/_sdata/_edata/_sbss/_ebss`，供启动代码或 C 初始化 `.bss` 使用。基址 `0x80000` 仅为教学占位，真板子按 SoC 手册修改。

## 工具速查

```bash
nm test.o
readelf -S kernel.elf
readelf -r test.o
ar rcs libtest.a a.o b.o
ld -T kernel.lds start.o -o kernel.elf
```
