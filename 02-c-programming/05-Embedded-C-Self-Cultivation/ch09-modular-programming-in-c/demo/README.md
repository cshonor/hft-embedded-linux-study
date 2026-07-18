# ch09 Demo

```bash
cd 00-Linux-Kernel-DPDK-Network-C/05-Embedded-C-Self-Cultivation/ch09-modular-programming-in-c/demo

make all
./demo01_minimal/demo01_minimal
./demo02_make/demo02_app
make mod_uart          # 仅重编 uart 静态库
make demo03            # CMake 构建
./demo03_cmake/build/demo03_cmake
./demo04_weak/demo04_weak
./demo04_weak/demo04_weak_board
./demo05_log_err/demo05_log_err
./demo06_callback/demo06_callback
```

## 目录结构

```
demo/
  common/          err.h log.h err.c libcommon.a
  demo01_minimal/  app/ driver/ utils/  — 最小三层
  demo02_make/     分层 Makefile + mod_uart + install
  demo03_cmake/    CMake add_library 链接
  demo04_weak/     platform weak + hw 强符号覆盖
  demo05_log_err/  统一日志与错误码
  demo06_callback/ sensor 回调解耦循环依赖
```

## demo02 单模块编译

```bash
make -C demo02_make mod_uart
make -C demo02_make mod_utils
make -C demo02_make install   # 输出到 demo/out/
```

## demo03 CMake

```bash
mkdir -p demo03_cmake/build && cd demo03_cmake/build
cmake ..
make
```

## .gitignore 建议

```
*.o *.a *.so demo*/demo*
build/
out/
```
