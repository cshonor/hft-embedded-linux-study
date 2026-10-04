# 02-ch16 code · 虚拟化域观测

> 《BPF Performance Tools》第 16 章示例落盘。
> 本机为裸金属（无 Hypervisor），Xen/KVM 工具以检测+速查形式归档；
> 笔记中 xenhyper.bt（§4 块8）只给了输出未给源码，无法补全。
> HFT 视角的虚拟化取舍见 [13-dpdk ch10-13](../../../13-dpdk/01-Intro-Book/)（裸金属 vs SR-IOV vs virtio）。

## scripts/

| 脚本 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04） |
|------|------|------|------|
| [hypervisor-detect.sh](./scripts/hypervisor-detect.sh) | §2 块1、§3 块2/7 | Hypervisor 检测（dmesg/systemd-detect-virt）+ perf kvm stat 读数 + Xen tracepoint 速查 + **PV vs HVM 教训**（工具没坏，是架构不同） | `bash -n` ✓；①段实跑 ✓（本机裸金属确认） |

## 关键认知（块7）

Xen HVM 下 `xen_mc*` 计数全为 0——不是工具坏了，是 HVM 用硬件虚拟化，
不走 paravirt 超级调用。**先确认虚拟化模式，再选工具集。**
