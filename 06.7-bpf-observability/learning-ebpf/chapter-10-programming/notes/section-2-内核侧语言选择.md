# 内核侧语言选择

- 可直接写 eBPF 字节码（Cloudflare 手写汇编的极端案例），实践上都是 **C 或 Rust** 编译目标
- **带运行时的语言不行**：GC 无法与验证器内存检查共存；eBPF 程序必须单线程，语言并发特性用不上（Go、Java 排除）
- GCC 10+ 也支持 eBPF 目标，但功能仍落后 LLVM
- XDPLua（内核跑 Lua 的 XDP）：研究结论性能不如 eBPF，且 eBPF 越来越强（循环等），无优势
