# 坑点清单

1. **vmlinux.h 没有 `#define` 常量**——协议号、标志位得自己补（第 8 章实例）
2. **vmlinux.h 撞名**——自定义 map/变量名可能和内核类型重名（config → my_config）
3. **`-O2` 不是可选项**——默认优化级别会产出 `callx`，验证器直接拒
4. **`-D __TARGET_ARCH_$(ARCH)` 忘了传**——用了 BPF_KPROBE 系宏就编译失败或取错寄存器
5. **加载后改 skel->data 无效**——配置必须在 open/load 之间做
6. **同机编译加载时重定位日志全是 0→0**——别误以为重定位没生效；跨内核才能看到真 patch
7. **libbpf 1.0 起节名严格**——老教程里的自由格式 SEC 名会加载失败
8. **perf buffer map 不带 BTF**——btf list 里看不到它属正常
9. **"无 BTF 内核"真实存在（rpi 官方内核实测）**——`/sys/kernel/btf/vmlinux` 不存在 ≠ eBPF 不可用：tracepoint/kprobe/map/ringbuf 全正常，只有 `vmlinux.h` 路线死了；libbpf 的 `failed to find valid kernel BTF` 日志是噪音，别被它劝退。绕行方案见 3.5
10. **新内核装好 ≠ 在运行**——`apt` 升级内核后 `uname -r` 仍显示旧版直到重启；查 BTF 前先确认跑的是哪个内核，别对着还没启用的镜像做判断
