# 第 7 章 · 探索 WebAssembly 物联网 (Exploring the Internet of WebAssembly Things)

> 所属：[07 Wasm+Rust](../README.md) · 目录：[Wasm-本书目录.md](../Wasm-本书目录.md)  
> **Layer 3 · 边缘**：[layer03](../layer03_quant-ma-strategy/README.md) · 前置：[第 6 章 · 控制台宿主](../chapter06_nonweb_hosts/README.md)

**章导读**：**GIMS** 通用指示器 Wasm 插件 · Battery/Pulsing 模块 · **ARM 交叉编译** · 树莓派 **blinkt 宿主**（mpsc · 10 FPS · cfg · **热重载**）。

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 | 状态 |
|:---:|------|------|:----:|
| **7.1** | Overview of the Generic Indicator Module | [7.1-indicator-overview.md](./7.1-indicator-overview.md) | ✅ |
| **7.2** | Creating Indicator Modules | [7.2-creating-indicators.md](./7.2-creating-indicators.md) | ✅ |
| **7.3** | Building Rust Applications for ARM Devices | [7.3-rust-arm.md](./7.3-rust-arm.md) | ✅ |
| **7.4** | Hosting Indicator Modules on a Raspberry Pi | [7.4-raspberry-pi-host.md](./7.4-raspberry-pi-host.md) | ✅ |
| **7.5** | Hardware Shopping List | [7.5-hardware-list.md](./7.5-hardware-list.md) | ✅ |
| **7.6** | Endless Possibilities | [7.6-endless-possibilities.md](./7.6-endless-possibilities.md) | ✅ |
| **7.7** | Wrapping Up | [7.7-wrap-up.md](./7.7-wrap-up.md) | ✅ |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；README 仅为索引。

---

## 本章速览

| 块 | 要点 |
|----|------|
| **GIMS** | sensor_update · apply · set_led |
| **模块** | 电池条 · Knight Rider 脉冲 |
| **部署** | armv7 cross · Pi + Blinkt |
| **宿主** | 10 FPS · 热重载 · cfg mock |
