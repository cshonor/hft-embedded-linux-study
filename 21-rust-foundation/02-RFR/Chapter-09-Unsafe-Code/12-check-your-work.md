# 4.3 Check Your Work（验证 unsafe 代码）

> 所属：**Coping with Fear** · [← 章索引](./README.md)

← [11 文档](./11-documentation.md) · 下一节 [13 小结](./13-summary.md)

前置 → [11 文档](./11-documentation.md) · [10 管理边界](./10-manage-boundaries.md) · [07 Panic 与不变式](./07-panics.md)

> ER → [Item 16 · 避免 unsafe](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/README.md)

---

手写 unsafe 仅靠肉眼极易遗漏 UB。须建立**多层自动化验证流水线** — 工具 + 测试 + 人工评审交叉校验，把内存违规、别名、生命周期、并发漏洞提前拦截。

```text
单元/集成测试  →  边界与 panic 路径兜底
Miri           →  Rust 专属 UB（首选）
ASAN/Valgrind  →  FFI / 堆栈越界
人工评审       →  最后一道防线（不可替代）
```

---

## 一、Miri（Rust 专属首选，最高优先级）

### 1. 基础

**Miri** = 编译器 **MIR 解释器**，逐指令模拟执行，全程追踪指针元数据、内存状态、别名规则、provenance，精准捕获仅 unsafe 才会触发的 UB — 普通 `cargo test` / release 运行往往**不暴露**。

**安装与执行**（nightly）：

```bash
rustup +nightly component add miri

cargo +nightly miri test          # 全部测试
cargo +nightly miri run           # 单独运行程序
cargo +nightly miri test -- test_unsafe_buf   # 指定用例
```

→ 本仓库 demo：[ER Item 16 miri](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/demo/)

### 2. Miri 可检测的典型 UB

| 类别 | 示例 |
|------|------|
| **内存违规** | 越界读写、use-after-free、双重释放、读未初始化、泄漏 |
| **对齐** | 裸指针转换后未对齐访问 |
| **Validity** | 非法 `bool`（非 0/1）、无效 enum 判别式 → [06 Validity](./06-validity.md) |
| **Strict Provenance** | `ptr → usize → ptr` 丢失 provenance 后解引用 |
| **别名（Stacked Borrows 等）** | `&mut` 别名违规、裸指针破坏借用规则 |
| **内置函数契约** | `copy_nonoverlapping` 重叠、`unreachable_unchecked` 实际可达 |
| **实验特性** | 多线程数据竞争、弱内存模型（视 Miri 版本） |

### 3. Miri 局限（须搭配其他工具）

- **不支持** FFI 调 C、SIMD、部分系统底层 API  
- **慢** — 不适合超大测试集；用于单元测试、边界用例  
- **无法**完全模拟真实机器堆分配器、底层内存行为  

→ §07 `bad_push_string` 类不一致窗口 UB，Miri 可观测

---

## 二、单元 / 集成测试（基础兜底，强制）

### 1. 覆盖要求

| 维度 | 用例 |
|------|------|
| **边界输入** | 空长度、0 容量、奇数对齐、极值、空指针、最大/最小分配 |
| **异常路径** | panic 分支、中途析构、提前释放、移动自引用结构 → [07 Panics](./07-panics.md) |
| **生命周期压力** | 短生命周期借用、提前 drop 依赖、多层嵌套引用 |
| **并发** | 多线程同时调 unsafe 接口（配合 sanitizer 查竞争） |

### 2. 设计原则

- unsafe 逻辑抽**独立测试模块**  
- 刻意构造「看似正常、实则 UB」的输入  
- 普通 release 可能**静默**；Miri / ASAN 应**直接报错**  

---

## 三、底层运行时检测（FFI / 高性能底层）

### 1. AddressSanitizer（ASAN）

| 项 | 说明 |
|----|------|
| **适用** | 纯 Rust + FFI、堆越界、栈溢出、释放后使用 |
| **启用** | `RUSTFLAGS="-Z sanitizer=address" cargo +nightly test`（需 nightly + 支持平台） |
| **优势** | 比 Valgrind 快；支持多线程；定位 C/Rust 交互内存错误 |

### 2. Valgrind（Linux）

| 项 | 说明 |
|----|------|
| **适用** | 跨语言 FFI、C 库泄漏、底层分配器异常 |
| **局限** | 主要 Linux；极慢；**不**识别 Rust 别名 / Stacked Borrows |

### 3. 工具分工

| 工具 | 擅长 | 短板 |
|------|------|------|
| **Miri** | Rust 别名、provenance、生命周期 UB | 不支持 C FFI |
| **ASAN** | 堆/栈越界、数据竞争、跨语言 | 不校验 Rust 借用模型 |
| **Valgrind** | 原生 C 泄漏、底层 ABI | 无 Rust 语义校验 |

---

## 四、人工代码评审（不可自动化替代）

### 1. 双人评审

所有 `unsafe {}`、`unsafe impl`、`unsafe fn` — **第二人复核**。

### 2. 评审核对清单

- [ ] 每条 unsafe 配套 `// SAFETY:` — 前置不变量完整 → [11 文档](./11-documentation.md)  
- [ ] `/// # Safety` 文档列出全部调用/实现约束  
- [ ] 结构体 `# Invariants` 稳定；所有修改路径维持不变量  
- [ ] 指针生命周期、对齐、所有权、并发同步无遗漏  

### 3. 评审重点

自引用结构 · 手动 `Send`/`Sync` → [04 unsafe trait](./04-unsafe-traits.md) · 裸指针内存管理 · **FFI 边界**

---

## 五、完整落地校验流程

| 阶段 | 动作 |
|------|------|
| **编码** | unsafe 同步写 `// SAFETY:` + 边界单元测试 |
| **提交前** | `cargo +nightly miri test` 无 UB |
| **CI** | stable 常规测试 + nightly Miri + ASAN（含 FFI 模块） |
| **上线前** | 核心底层 unsafe 模块双人评审 |

缩小 unsafe 范围 → 减少评审/测试量 → [10 管理边界](./10-manage-boundaries.md)

---

## 六、联动知识点

| 来源 | 要点 |
|------|------|
| ER Item 16 | 优先消除 unsafe；无法消除则**全套**验证 |
| §10 边界 | 缩小信任域，降低审计面 |
| §11 文档 | SAFETY 注释是 Miri 报错后的定位依据 |
| Book 19.1 | [不安全 Rust](../../00-Book/19-advanced-features/19.1-不安全Rust.md) · [demo](../../00-Book/19-advanced-features/19.1-unsafe-rust-demo/) |

---

## 七、核心总结

1. **Miri** 是 Rust unsafe **首选** — 别名、provenance、未初始化、悬垂等独有 UB。  
2. Miri 仅 **nightly**；不支持 C FFI → 搭配 ASAN / Valgrind。  
3. **三层自动化**：单元测试 → Miri → Sanitizer / Valgrind。  
4. **人工评审不可替代** — unsafe 强制双人复核。  
5. 测试须覆盖边界、panic、短生命周期、并发等高危路径。

---

## 速记

## 四层验证（推荐顺序）

1. **单元/集成测试** — 边界、panic、生命周期、并发  
2. **Miri** — Rust UB 首选（nightly）  
3. **ASAN / Valgrind** — FFI、堆栈越界  
4. **人工评审** — 双人复核 unsafe  

## Miri 命令

```bash
rustup +nightly component add miri
cargo +nightly miri test
```

## Miri 擅长

越界 · UAF · 未初始化 · Validity · provenance · Stacked Borrows

## Miri 短板

无 C FFI · 慢 · 非真实分配器

## 工具分工

| Miri | ASAN | Valgrind |
|------|------|----------|
| Rust 语义 UB | 堆/栈/竞争/跨语言 | C 泄漏/ABI |

## 评审清单

`// SAFETY:` · `# Safety` · `# Invariants` · 生命周期/对齐/所有权/并发

## 自测

- [ ] 为何 release `cargo test` 不够，还须 Miri？  
- [ ] FFI 模块为何须 ASAN 而非只靠 Miri？  
- [ ] Miri 报错时如何借助 §11 文档定位契约？

