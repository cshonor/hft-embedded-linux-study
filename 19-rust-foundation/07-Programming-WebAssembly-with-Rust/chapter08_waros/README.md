# 第 8 章 · 构建 WARoS——WebAssembly 机器人系统

> 所属：[07 Wasm+Rust](../README.md) · 目录：[Wasm-本书目录.md](../Wasm-本书目录.md) · **Layer 3**：[策略引擎映射](../layer03_quant-ma-strategy/README.md)  
> 前置：[第 6 章 · 宿主](../chapter06_nonweb_hosts/README.md) · [第 7 章 · IoT](../chapter07_iot/README.md)

**章导读**：致敬 **Crobots** · **WARoS API (FFI)** · **ECS 比赛引擎** · **dumbotrs / Rook** · **consolerunner 10 万帧回放** · **云端 Kafka/WebSocket** — 全书 Rust/Wasm 最深实战。

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 | 状态 |
|:---:|------|------|:----:|
| **8.1** | An Homage to Crobots | [8.1-homage-crobots.md](./8.1-homage-crobots.md) | ✅ |
| **8.2** | Designing the WARoS API | [8.2-waros-api.md](./8.2-waros-api.md) | ✅ |
| **8.3** | Building the WARoS Match Engine | [8.3-match-engine.md](./8.3-match-engine.md) | ✅ |
| **8.4** | Creating WebAssembly Robots | [8.4-wasm-robots.md](./8.4-wasm-robots.md) | ✅ |
| **8.5** | Robots in the Cloud | [8.5-robots-cloud.md](./8.5-robots-cloud.md) | ✅ |
| **8.6** | Wrapping Up | [8.6-wrap-up.md](./8.6-wrap-up.md) | ✅ |
| **8.7** | Conclusion | [8.7-conclusion.md](./8.7-conclusion.md) | ✅ |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；README 仅为索引。

---

## 本章速览

| 块 | 要点 |
|----|------|
| **规则** | 1000×1000 · 雷达 700m · 双导弹 |
| **API** | scan · cannon · drive · damage · trig |
| **引擎** | ECS · wasmi · Game Loop |
| **机器人** | dumbotrs · Rook · consolerunner 100k |
| **云** | FaaS · Kafka · WebSocket |
