# 4.2 Read and Write Documentation（文档）

> 所属：**Coping with Fear** · [← 章索引](./README.md)

← [10 管理边界](./10-manage-boundaries.md) · 下一节 [12 验证工作](./12-check-your-work.md)

前置 → [10 管理边界](./10-manage-boundaries.md) · [07 Panic 与不变式](./07-panics.md)

> ER → [Item 27 文档公开接口](../../01-ER/Chapter-05-Tooling/Item-27-document-public-api/README.md)

---

unsafe 最大风险之一是**人工契约无书面记录**。本章强制区分**两套安全文档体系** — 不可混淆：

```text
/// # Safety   →  对外 API 契约（告诉调用者须担保什么）
// SAFETY:     →  对内局部证明（说明当前上下文为何满足前提）
```

---

## 一、两套安全文档体系（强制二分）

### 1. 行内 `// SAFETY:` — 代码内部 unsafe 块证明

| 项 | 说明 |
|----|------|
| **作用对象** | 每一处 `unsafe {}`、`unsafe impl`、调用 `unsafe fn` 的内部代码 |
| **位置** | 紧贴 unsafe 代码**上方** |
| **格式** | `// SAFETY:` 开头，分两层 |

**两层内容**：

1. **外部前置约束** — 调用方/上下文须满足的不变量（指针有效、对齐、生命周期、无数据竞争、所有权等）  
2. **本地证明** — 当前代码上下文如何保证上述前提**全部成立**

```rust
pub fn push(&mut self, val: T) {
    if self.len == self.cap {
        self.grow();
    }
    // SAFETY:
    // 1. self.ptr 由分配器分配，容量至少 self.cap 个 T，内存对齐合法
    // 2. self.len < self.cap，写入位置未初始化、无有效值无需 drop
    // 3. 独占 &mut self，不存在并发访问数据竞争
    unsafe {
        self.ptr.add(self.len).write(val);
    }
    self.len += 1;
}
```

### 2. rustdoc `/// # Safety` — 对外公开 API 契约

| 项 | 说明 |
|----|------|
| **作用对象** | `unsafe fn`、`unsafe trait`、`unsafe impl`、对外底层接口 |
| **受众** | 外部使用者、库调用者、其他 crate 开发者 |
| **内容** | 调用者/实现者须**手动担保**的全部安全契约 — 编译器不校验，违反即 UB |

**配套区块**：`# Panics` · `# Invariants`（类型不变量）· `# Examples`

```rust
/// 从裸指针读取 `i32` 值。
///
/// # Safety
///
/// 调用者必须同时满足全部条件：
/// 1. `ptr` 非空、对齐符合 `i32` 要求
/// 2. `ptr` 指向已初始化有效内存
/// 3. `ptr` 生命周期长于本次读取，无并发修改
pub unsafe fn read_i32(ptr: *const i32) -> i32 {
    // SAFETY: 函数文档强制调用者满足全部约束，此处可安全解引用
    unsafe { *ptr }
}
```

### 二者对照

| 维度 | `// SAFETY:` 行内 | `/// # Safety` rustdoc |
|------|-------------------|------------------------|
| 面向人群 | 维护者、读实现源码的人 | 外部使用者、API 调用者 |
| 作用范围 | 单段 unsafe 局部证明 | 整个函数/trait 对外契约 |
| 核心目的 | 证明**当前上下文满足**安全前提 | 告知**使用者须遵守**哪些规则 |
| 位置 | 代码块上方 | 函数/结构体/trait 文档 |
| 依赖关系 | 依赖 rustdoc 定义的前提 | **定义**前提，是行内注释的依据 |

---

## 二、ER Item 27 核心规则

1. **所有公开接口须完整文档** — unsafe 相关约束为最高优先级  
2. `unsafe fn` **强制** `# Safety` — 遗漏属严重工程缺陷  
3. 区分三类文档区块：

| 区块 | 用途 |
|------|------|
| `# Safety` | 仅 unsafe 相关前置约束 |
| `# Panics` | 安全函数会 panic 的输入边界 |
| `# Invariants` | 结构体长期维持的内存不变量（供内部 unsafe 引用） |

4. **安全包装器**（safe API 包底层 unsafe）须在文档说明内部依赖的不变量  
5. 文档示例可编译执行（`cargo test` / doctest）— 避免契约与代码脱节  

→ [ER Item 27](../../01-ER/Chapter-05-Tooling/Item-27-document-public-api/README.md)

---

## 三、三类 unsafe 场景文档规范

### 场景 1：unsafe 被 safe 函数包裹（最常见）

1. 对外 safe 函数 rustdoc 写 **`# Invariants`**（内部维护的不变量）  
2. 内部每处 `unsafe {}` 加 `// SAFETY:`，引用不变量证明合法  
3. 外部调用者无需写 unsafe；仅维护者须看懂行内注释  

→ 与 [10 管理边界](./10-manage-boundaries.md)「缩小边界」一致

### 场景 2：对外暴露 `unsafe fn`

1. rustdoc **`# Safety`** 完整列出全部调用前提 — 无模糊描述  
2. 内部 `// SAFETY:` 说明「调用者已遵守文档约束，因此操作安全」  
3. **禁止**省略任何一条前置条件  

### 场景 3：`unsafe trait` / `unsafe impl`

1. trait 定义处 `# Safety`：实现者须维持的全局不变量  
2. `unsafe impl` 上方 `// SAFETY:` 逐条证明当前类型满足 trait 契约  
3. 标准库 `Send` / `Sync` 均遵循此规范 → [04 unsafe trait](./04-unsafe-traits.md)

---

## 四、`// SAFETY:` 硬性标准（社区约定）

1. 每条 unsafe 操作**单独**配套一条 — 不合并多个块共用一条  
2. 语言清晰：明确写出有效 / 对齐 / 生命周期 / 独占访问 / 初始化状态  
3. **禁止**仅写「this is safe」等无审计价值的短句  
4. 结构体长期不变量写在结构体 `# Invariants`；行内注释直接引用  
5. 开源库、Rust for Linux 等：**无 SAFETY 注释不予合入**  

---

## 五、工程价值

| 价值 | 说明 |
|------|------|
| 可审计凭证 | 人工契约的唯一书面记录 |
| 责任分离 | 对外约束使用者，对内证明实现合规 |
| 审查 / Miri / 重构 | SAFETY 注释是判断 sound 的核心依据 |
| 长期维护 | 数月后改 unsafe 代码可快速复现当年安全逻辑 |

→ 验证手段：[12 验证工作](./12-check-your-work.md) · Miri

---

## 六、核心总结

1. **两套文档**：`/// # Safety` 对外契约，`// SAFETY:` 对内证明。  
2. `unsafe fn` 必须 rustdoc Safety；每个 unsafe 块必须行内 SAFETY。  
3. 行内 SAFETY 两点：**前置约束** + **当前如何满足**。  
4. ER Item 27：公开接口完整文档，区分 Safety / Panics / Invariants。  
5. 安全包装器：对外写不变量，内部 unsafe 引用不变量证明合法性。

---

## 速记

## 两套体系（不可混）

| | `/// # Safety` | `// SAFETY:` |
|---|----------------|--------------|
| 对象 | 对外 API 契约 | 对内局部证明 |
| 受众 | 调用者 | 维护者 |
| 定义/证明 | **定义**前置条件 | **证明**当前满足 |

## 行内 SAFETY 两层

1. 外部前置约束（指针/对齐/生命周期/竞争/所有权）  
2. 本地证明（上下文如何保证全部成立）

## ER Item 27 三区

- `# Safety` — unsafe 前置约束  
- `# Panics` — 安全函数 panic 边界  
- `# Invariants` — 结构体长期不变量  

## 三场景

1. safe 包装 → `# Invariants` + 内部 `// SAFETY:`  
2. `unsafe fn` → 完整 `# Safety` + 内部引用契约  
3. `unsafe trait/impl` → trait `# Safety` + impl 上 `// SAFETY:`  

## 硬性标准

一条 unsafe 一条注释 · 禁「this is safe」· 不变量集中写 struct 文档

## 自测

- [ ] 行内 SAFETY 与 rustdoc Safety 谁定义、谁证明？  
- [ ] safe 包装器为何要在 rustdoc 写 Invariants？  
- [ ] 为何不能多个 `unsafe {}` 共用一条 SAFETY？

