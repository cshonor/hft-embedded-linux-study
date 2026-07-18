# ch04 Demo

## 四阶段编译

```bash
make demo01
readelf -S demo01.o
readelf -S demo01
```

## 静态库 / 动态库

```bash
make demo_static demo_shared
./demo_static
./demo_shared
nm libdemo.a
ldd demo_shared
```

## 工具

```bash
readelf -h demo01
objdump -h demo01
nm demo01.o
```
