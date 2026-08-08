# Projects — Project 驱动学习路线

> **理念：** 不是"先读完书再做项目"，而是**项目本身就是学习路径**——卡住了翻书查对应模块，做完就自然学会了。  
> 上层路线与模块覆盖 → [../README.md#project-驱动学习路线](../README.md)

## 依赖链

```
P1 CPU 模拟器
  ↓
P2 Shell + malloc
  ↓
P3 并发 HTTP Server ──────────┐
  ↓                            │
P4 内核模块                    │
  ↓                            │
P5 树莓派嵌入式（P5a–P5e）      │
  │                            │
  │   P6 网络协议分析器 ←───────┘
  │     ↓
  │   P7 DPDK 转发 + 延迟剖析
  │     │
  └─────┴──→ P8 迷你撮合引擎（终极大作业）
```

## 项目清单

| Project | 做什么 | 覆盖模块 | 前置 | 状态 |
|:-------:|--------|:--------:|:----:|:----:|
| [P1](./P1-cpu-simulator/) | 8-bit CPU（ALU+寄存器+FSM） | `00` | 无 | ⬜ 未开始 |
| [P2](./P2-shell-malloc/) | mini shell + 自制 malloc/free | `01` `02` | P1 | ⬜ 未开始 |
| [P3](./P3-http-server/) | 并发 HTTP Server（C → C++ 重写） | `04` `05` `06` | P2 | ⬜ 未开始 |
| [P4](./P4-kernel-module/) | 可加载内核模块（字符设备+kmalloc+/proc） | `07` `08.5` `08.6` `09` | P3 | ⬜ 未开始 |
| [P5](./P5-raspberry-pi-embedded/) | 树莓派嵌入式全链路（5 子项目） | `10`–`14` | P4 | ⬜ 未开始 |
| [P6](./P6-network-protocol-analyzer/) | 抓包+逐层解析+TCP 流重组+eBPF | `15` `16` `17` `17.5` `20` | P3 | ⬜ 未开始 |
| [P7](./P7-dpdk-forwarder-profiling/) | DPDK forwarder+perf 火焰图+bpftrace | `18` `19` `20` | P6 | ⬜ 未开始 |
| [P8](./P8-matching-engine/) | 限价订单簿撮合引擎+无锁+Rust 重写 | `21` `22` `23` | P4+P5+P7 | ⬜ 未开始 |

> 状态标记：⬜ 未开始 / 🔄 进行中 / ✅ 完成

## 约定

- 每个 Project 文件夹放 `README.md`（本脚手架）+ `src/`（代码）+ `notes/`（踩坑记录）+ `refs/`（资料链接）。
- 代码可放本仓库或拆到独立仓库，本目录至少保留 README + 笔记索引。
- Project 编号 `P1`–`P8` 与模块编号 `00`–`23` 是两套体系：**模块=知识，Project=产出**。
