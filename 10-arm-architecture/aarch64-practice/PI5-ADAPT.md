# 树莓派 5 适配本书实验（总览）

> 原书基准：**Pi4B · BCM2711 · Cortex-A72**  
> 本仓库硬件：**Pi5 · BCM2712 · Cortex-A76 · ARMv9-A**  
> 详记（Ch2）：[chapter-02 … / section-0-Pi5适配与实验路线.md](./chapter-02-raspberry-pi-lab/notes/section-0-Pi5适配与实验路线.md)

---

## 一句话

**A64 / EL / MMU / GIC 逻辑通用；PL011·GIC 基址、`config.txt`、JTAG 不同 → 架构实验主力 QEMU（`-cpu cortex-a76`），外设再迁 Pi5。**

```bash
qemu-system-aarch64 -M virt -cpu cortex-a76 -nographic -kernel benos.bin -s -S
```

勿把 4B BenOS bin 原样丢进 Pi5 boot。工具链仍是 `aarch64-linux-gnu-*`。
