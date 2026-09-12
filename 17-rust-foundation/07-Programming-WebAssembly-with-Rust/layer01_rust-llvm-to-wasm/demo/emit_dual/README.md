# emit_dual · 同一份 Rust → `.ll` + `.wasm`

> Layer 1 的练手工程。**目的不是写功能，而是建立「Rust 源码 → 各阶段产物」的对照习惯**——
> 与 [06 llvm_insight_lab](../../../../06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md)
> 是同一套打法，只是目标三元组从 host 换成 `wasm32-unknown-unknown`。

## 一条命令出四种产物

```bash
cd 07-Programming-WebAssembly-with-Rust/layer01_rust-llvm-to-wasm/demo/emit_dual

cargo test                                              # 4 个单测，先钉住语义

# ① LLVM IR（host triple，O0）：看源码怎么被直译
cargo rustc -- --emit=llvm-ir
#    → target/debug/deps/emit_dual.ll        （约 118 KB，全是没优化的代码）

# ② LLVM IR（host triple，O3）：看优化后剩什么
cargo rustc --release -- --emit=llvm-ir
#    → target/release/deps/emit_dual.ll      （约 6.5 KB）

# ③ Wasm 二进制
cargo build --release --target wasm32-unknown-unknown
#    → target/wasm32-unknown-unknown/release/emit_dual.wasm   （约 827 B）

# ④ WAT（可读文本，需要 wabt / wasm-tools）
wasm2wat target/wasm32-unknown-unknown/release/emit_dual.wasm -o emit_dual.wat
# 或：wasm-tools print emit_dual.wasm
```

> `target/` 已在 `.gitignore` 中，产物不入库 —— 需要时按上面命令重新生成。

---

## 两个被导出的函数

| 函数 | 特征 | 对照看点 |
|------|------|----------|
| `sma_update` | 纯算术，无内存访问 | 寄存器上的 `fsub`/`fadd`/`fdiv` |
| `weighted_mean` | 循环 + 线性内存读取 | **i32 偏移寻址**、循环头、边界检查 |

两个都标了 `#[no_mangle] extern "C"`，所以在 `.wasm` 的 export section 里能看到本名
（可用 `wasm2wat` 或 `wasm-objdump -x` 验证）。

### O3 下 `sma_update` 的 IR（实测摘录）

```llvm
define noundef double @sma_update(double noundef %sum, i32 noundef %len,
                                  double noundef %out_price, double noundef %in_price) #0 {
start:
  %0 = icmp eq i32 %len, 0
  %_6 = fsub double %sum, %out_price
  %_5 = fadd double %_6, %in_price
  %_7 = uitofp i32 %len to double
  %1 = fdiv double %_5, %_7
```

要点：

- 源码里的 `if len == 0 { return 0.0 }` **没有生成分支**，被折叠成 `icmp` + 后续的 `select`
  （LLVM 把「条件返回」重构成了数据流）
- `len as f64` 是 `uitofp` —— **无符号**转换，因为 `u32` 不会是负数
- 没有任何 `load`/`store`：参数全在寄存器里，这正是「纯算术函数」的特征

对照组：把同一函数拿到 `weighted_mean` 那边看，会出现 `getelementptr` + `load`，
因为要按索引读线性内存。

---

## 需要回答的四个问题（Layer 1 验收）

1. `wasm32-unknown-unknown` 与 host triple 的 **std** 差异？
   → 该 target 没有完整 `std`（无文件系统、无网络、无线程），只有 `core` + 部分 `alloc`。
   这也是为什么本 demo 刻意只写纯计算。
2. 线性内存里 **指针是 i32 偏移** —— 与 Nomicon / RFR 的布局知识如何对应？
   → Wasm 是 32 位地址空间，`*const f64` 在 `.wat` 里表现为 `i32`，按 `8` 字节步进。
   对比 host 下的 64 位指针，这是最直观的一处差异。
3. 同一循环在 `.ll` 与 `.wat` 里各看到什么 **load/store** 模式？
   → `.ll` 是 `getelementptr` + `load f64`；`.wat` 是 `i32.add`(算偏移) + `f64.load`。
4. 哪些优化在 **LLVM 后端** 完成、哪些在 **wasm-opt** 完成？
   → 内联、常量折叠、循环展开在 LLVM（`opt-level=3` 已生效，见上面 .ll 体积 118KB→6.5KB）；
   `wasm-opt` 额外做的是 Wasm 特有的：死代码消除（删未导出函数）、指令合并、体积压缩。

---

## HFT 关联

- `sma_update` 是**滚动均价的 O(1) 增量更新**——真实策略里就是这么避免每 tick 重算窗口的
- `weighted_mean` 的「线性内存 + i32 偏移」是理解 Wasm 数值代码性能的关键：
  没有 SIMD 时，逐元素加权就是一串 `f64.load` + `f64.mul` + `f64.add`
- 本层的真正价值：**知道 `cargo build --target wasm32` 之后到底发生了什么**，
  而不是把 Wasm 当黑盒

→ 下一层：[Layer 2 · 订单簿查询 Wasm](../../layer02_orderbook-query-wasm/README.md)
