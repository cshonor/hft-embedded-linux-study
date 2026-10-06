# ch04 Demo

```bash
make all
./demo01_extern/main
./demo02_extern_type/use_wrong    # 乱码/短输出
./demo02_extern_type/use_correct
./demo03_static_lib/demo
./demo04_bss_data/main
make -C demo05_undef link_fail    # undefined reference to helper
```
make -C demo06_unused_static link_fail_ext    # 外部链接未用函数：必报 undefined reference
make -C demo06_unused_static link_static_O0   # static 未用：gcc -O0 报错 / clang 消除通过
make -C demo06_unused_static link_static_O2   # static 未用：-O2 消除，通过
