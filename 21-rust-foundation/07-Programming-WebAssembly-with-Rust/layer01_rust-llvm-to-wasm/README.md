# Layer 1 · Rust + LLVM → Wasm 编译链路

> 上层索引：[三层学习架构](../三层学习架构.md) · 原书：**第 1 章** [WebAssembly Fundamentals](../chapter01_wasm_fundamentals/README.md)

**目标**：把 **06 Learn LLVM 17** 里已熟悉的 `rustc --emit=llvm-ir` 延伸到 **`wasm32-unknown-unknown`**，建立「同一 Rust 函数 → `.ll` / `.wasm` / WAT」的对照习惯。

---

## 核心子主题（原书第 1 章）

| 节 | 主题 | 本层动作 |
|:---:|------|----------|
| 1.1 | Introducing WebAssembly | Wasm 模块边界 · 与 native 目标对比 |
| 1.2 | Understanding WebAssembly Architecture | 栈式 VM · 线性内存 ↔ LLVM load/store |
| 1.3 | Building a WebAssembly Application | 工具链跑通首个 `.wasm` |
| 1.4 | Wrapping Up | 记录「Rust 源码 → 各阶段产物」清单 |

笔记 → [chapter01_wasm_fundamentals/](../chapter01_wasm_fundamentals/README.md)

---

## 编译链路（与 06 复用）

```text
Rust 源码 (lib.rs)
    ├─ cargo rustc -- --emit=llvm-ir     →  *.ll   （06 llvm_insight_lab 同款）
    ├─ cargo build --target wasm32-unknown-unknown  →  *.wasm
    └─ wasm2wat / wasm-tools parse       →  *.wat   （可读中间层，对照 .ll）
         optional: wasm-opt -O3          →  体积/指令数 diff
```

### 前置（06 专题）

| 资源 | 用途 |
|------|------|
| [llvm_insight_lab](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md) | 已有 IR 导出流程 |
| [ir_samples/](../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/ir_samples/README.md) | O0/O3、atomic 片段对照 |
| [CI · VM 地图](../../06_Compilers-and-LLVM-Learning/01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/01-7-virtual-machine.md) | 栈式 VM 直觉 |

### 工具链（本层安装清单）

```bash
rustup target add wasm32-unknown-unknown
# 可选：binaryen (wasm-opt, wasm2wat), wasm-tools, trunk/wasm-pack（Layer 2 再用）
```

---

## 练手 demo（规划）

目录：`layer01_rust-llvm-to-wasm/demo/`

| demo | 说明 | 状态 |
|------|------|:----:|
| `emit_dual/` | 同一 `add_moving_average_tick()` 导出 `.ll` + `.wasm` | 📄 待建 |
| `wat_diff/` | O0 wasm vs `wasm-opt -O3` 指令数对比 | 📄 待建 |

**建议源码**：从 Layer 3 会用的「单 tick 均线更新」抽一个**纯计算**函数，避免 Layer 1 就引入 JS/网络。

---

## 输出检查清单

刷完 Layer 1 应能口头回答：

1. `wasm32-unknown-unknown` 与 host triple 的 **std** 差异？
2. 线性内存里 **指针是 i32 偏移** — 与 Nomicon / RFR 布局如何对应？
3. 同一循环在 `.ll` 与 `.wat` 里各看到什么 **load/store** 模式？
4. 哪些优化在 **LLVM 后端** 完成、哪些在 **wasm-opt** 完成？

---

## 下一层

→ [Layer 2 · 订单簿查询 Wasm](../layer02_orderbook-query-wasm/README.md)（原书第 2～4 章 · 紧凑数据结构 + bindgen）
