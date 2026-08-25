# Projects — Project 驱动学习路线

> **理念：** 笔记是地图，项目是路——**先上路，卡住了再查模块笔记**；不是「读完 2000 篇再动手」。  
> P1 已写成带「卡住翻哪篇」索引的实战指南 → [P1-cpu-simulator/README.md](./P1-cpu-simulator/README.md)  
> **构建：** [CMake vs Makefile](./CMAKE-VS-MAKEFILE.md)（CMake 不是 make 的插件）  
> 上层路线 → [../README.md](../README.md)

## 依赖链

```
P1 CPU 模拟器
  ↓
P2 Shell + malloc
  ↓
P2.5 C 工具箱（GNU C 桥梁）
  ↓
P3 并发 HTTP Server ──────────┐
  ↓                            │
P3.5 BusyBox 极简 Linux         │
  ↓                            │
P4 内核模块                    │
  ↓                            │
P5 树莓派嵌入式（P5a–P5f）      │
  │                            │
  │   P6 网络协议分析器 ←───────┘
  │     ↓
  │   P7 DPDK 转发 + 延迟剖析
  │     │
  └─────┴──→ P8 迷你撮合引擎（终极大作业）
               ↓
             P10 HFT 单机原型（part-a demo 已可跑）
```

## 项目清单

| Project | 做什么 | 覆盖模块 | 前置 | 状态 |
|:-------:|--------|:--------:|:----:|:----:|
| [P1](./P1-cpu-simulator/) | 8-bit CPU（ALU+寄存器+FSM） | `00` | 无 | ✅ `make test` |
| [P2](./P2-shell-malloc/) | mini shell + 自制 malloc/free + C 特性练手 | `01` `02` | P1 | ✅ 管道/后台/`&` + 显式链表 malloc |
| [P2.5](./P2.5-c-toolkit/) | GNU C 工具箱：container_of + 侵入式链表 + ring buffer | `01` | P2 | ✅ `part-a-toolkit make test` |
| [P3](./P3-http-server/) | 并发 HTTP Server（C → C++ 重写） | `19` `05` `04` | P2 | 🔄 GET 200/404 + echo（epoll/线程池未做） |
| [P3.5](./P3.5-busybox-minimal-linux/) | BusyBox 极简 Linux（内核编译+rootfs+启动链） | `05` `08` | P3 | ⬜ 指南在；需本机 QEMU 编内核 |
| [P4](./P4-kernel-module/) | 可加载内核模块（字符设备+kmalloc+/proc） | `05` `05.5` `05.6` `06` | P3+P3.5+P2.5 | 🔄 hello 源码有；WSL `make test` 只跑用户态 |
| [P5](./P5-raspberry-pi-embedded/) | 树莓派嵌入式全链路（6 子项目） | `07`–`11` | P4 | ⬜ 需树莓派/QEMU，本机不假装完成 |
| [P6](./P6-network-protocol-analyzer/) | 抓包+逐层解析+TCP 流重组+eBPF | `12` `13` `14` `14.5` `17` | P3 | 🔄 合成帧解析+pcap；raw socket/eBPF 未做 |
| [P7](./P7-dpdk-forwarder-profiling/) | DPDK forwarder+perf 火焰图+bpftrace | `15` `16` `17` | P6 | 🔄 `part-a-host-poll` 模拟 PMD 循环；真 DPDK 需网卡 |
| [P8](./P8-matching-engine/) | 限价订单簿撮合引擎+无锁+Rust 重写 | `18` `21` `22` | P4+P5+P7 | 🔄 `part-a-lob` 正确性（撤单/部分成交）；全链路见 P10 / 18-rust-quant |
| [P9](./P9-os-from-scratch/) | OS 从零造（MikanOS + 30 天精华） | ~~05~~ (拓展) | Phase4 完成 | ⬜ 笔记为主 |
| [P10](./P10-hft-prototype/) | HFT 单机原型：demo 全链路（一文 + part-a） | `14` `19` | P8 规划 | ✅ part-a demo |

> 状态标记：⬜ 未开始 / 🔄 进行中 / ✅ 完成

## 约定

- 有 Part 的项目：代码进可 `make` 的 `part-*` 子工程（如 `part-a-shell/`），指南用 `Part-*.md`。
- **不要**再放空的 `src/` / `notes/` / `refs/` 占位目录。
- Project 编号与模块编号是两套体系：**模块=知识，Project=产出**。
WSL 一键：

```bash
cd projects && make test
```

### 已拆成可运行子工程的例子

| Project | 子工程 |
|---------|--------|
| P1 | `part-a-alu-host` · `part-b-multicycle` |
| P2 | `part-a-shell` · `part-b-malloc` · `part-c-exercises` |
| P2.5 | `part-a-toolkit` |
| P3 | `part-a-c-server` · `part-b-cpp-rewrite` |
| P4 | `part-a-hello-chardev`（`make test`=用户态） |
| P6 | `part-a-parser` |
| P7 | `part-a-host-poll` |
| P8 | `part-a-lob` |
| P10 | `part-a-demo`（CMake 也可） |
