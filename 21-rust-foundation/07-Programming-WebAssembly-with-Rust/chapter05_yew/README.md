# 第 5 章 · 使用 Yew 进行高级 JavaScript 集成 (Advanced JavaScript Integration with Yew)

> 所属：[07 Wasm+Rust](../README.md) · 目录：[Wasm-本书目录.md](../Wasm-本书目录.md)  
> **Layer 3**：[layer03 · Yew 回测 SPA](../layer03_quant-ma-strategy/README.md) · 前置：[第 4 章 · wasm-bindgen](../chapter04_js_integration/README.md)

**章导读**：用 **Yew**（虚拟 DOM · 组件 · `Msg`）构建几乎全 Rust 的浏览器 SPA；计数器入门后，以 **PubNub ChatEngine + Services + 防腐层** 实现实时聊天 — 工程化前端里程碑。

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 | 状态 |
|:---:|------|------|:----:|
| **5.1** | Getting Started with Yew | [5.1-getting-started-yew.md](./5.1-getting-started-yew.md) | ✅ |
| **5.2** | Building a Live Chat Application | [5.2-live-chat-app.md](./5.2-live-chat-app.md) | ✅ |
| **5.3** | Wrapping Up | [5.3-wrap-up.md](./5.3-wrap-up.md) | ✅ |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；README 仅为索引。

---

## 本章速览

| 块 | 要点 |
|----|------|
| **Yew** | Virtual DOM · Component · `html!` · `Msg` / `update` |
| **计数器** | 首个应用 · 熟悉工作流 |
| **聊天 SPA** | ChatEngine (JS) · Services · 防腐层 · 组件树 |
| **Layer 3** | 聊天 UI → **回测仪表盘** |
