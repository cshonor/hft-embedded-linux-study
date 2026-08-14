# 4.1 Manage Unsafe Boundaries（管理 unsafe 边界）

> 所属：**Coping with Fear** · [← 章索引](./README.md)

← [09 Drop 检查](./09-drop-check.md) · 下一节 [11 文档](./11-documentation.md)

前置 → [05 UB 与三大契约](./05-what-can-go-wrong.md) · [01 unsafe 双重角色](./01-unsafe-keyword.md)

> ER → [Item 16 · 避免 unsafe](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/README.md)

---

unsafe 最大风险是**安全不变量泄露** — 通过三层边界策略收缩**信任域**，把人工担保的代码压到最小范围。

```text
缩小边界     →  unsafe 私有，对外只暴露 Safe API
同一信任域   →  改不变量的代码全在边界内
优先安全替代 →  能不写 unsafe 就不写
```

---

## 一、缩小边界：私有模块 + 安全 API

### 核心逻辑

unsafe 是**内部实现细节** — **不**向外泄露裸指针、未校验接口、`pub unsafe fn`（除非刻意且文档化）。  
所有危险操作藏在私有字段/私有函数；公共接口全是 **safe**，调用者**零**担保责任。

### 落地规范

| # | 规范 |
|:-:|------|
| 1 | 裸指针、底层内存字段 **`private`** |
| 2 | 危险逻辑 → 模块内 `unsafe fn`（`pub(crate)` 或私有） |
| 3 | 公共 safe 方法：**先校验**（长度、非空、对齐、生命周期），再进**极小** `unsafe {}` |
| 4 | 模块隔离：FFI/分配器单独 `mod`，上层业务不碰 unsafe |

### 极简示例

```rust
mod buffer {
    pub struct Buffer<T> {
        ptr: *mut T,   // 私有
        len: usize,
        cap: usize,
    }

    impl<T> Buffer<T> {
        pub fn push(&mut self, val: T) {
            if self.len == self.cap {
                self.grow();
            }
            // 前置：len < cap，ptr 有效
            unsafe {
                self.ptr.add(self.len).write(val);
            }
            self.len += 1;
        }

        fn grow(&mut self) {
            // 模块内：重分配等 unsafe 集中在此
            todo!("realloc + update ptr/cap");
        }
    }
}
```

### 反面风险

| 做法 | 后果 |
|------|------|
| `pub ptr: *mut T` | 外部随意破坏不变量 |
| `pub unsafe fn get_unchecked(...)` | 安全契约外泄 |
| 大块 `unsafe {}` 无前置校验 | 审计困难、易 UB |

→ 文档化前提：[11 文档](./11-documentation.md) · Miri：[12 验证](./12-check-your-work.md)

---

## 二、同一信任域：不变量修改收拢

### 不变量（Invariant）

unsafe 依赖的一组**人工契约**，例如：

- 指针非空、对齐、指向已分配区域  
- `len ≤ cap`，`0..len` 已初始化且 Valid  
- 所有权唯一、无数据竞争  

### 信任域规则

**所有能修改、破坏该不变量的代码** → 同一私有模块 / 同一 `impl` 信任域内。

| | |
|---|---|
| ✅ 允许 | 同模块私有方法改 `ptr` / `len` / `cap` |
| ❌ 禁止 | 外部模块直接写字段；跨 crate 裸指针篡改内部状态 |

### 原理

不变量修改**集中** → 审计只盯一小块；状态修改**分散** → 每次改动都要全量重审，极易漏洞。

→ 与 [07 Panic](./07-panics.md) 一致：不一致窗口也须在信任域内用 Guard/`MaybeUninit` 管理。

---

## 三、优先安全替代（ER Item 16）

**尽量不要自己手写 unsafe。**

| 层级 | 选择 |
|------|------|
| 标准库 | `Vec` / `Box` / `Arc` / `Mutex` / slice API — 底层经 Miri/生产验证 |
| 成熟 crate | `bytemuck`（安全 transmute 子集）、`crossbeam` 等 — 视场景 |
| 手写 unsafe | 仅当：自定义分配器、FFI、硬件映射、特殊无锁结构等**生态无解** |

### 取舍流程

```text
1. 能否纯 Safe Rust 重构？        → 优先，消除 unsafe
2. 标准库有无安全抽象？          → 直接复用
3. 社区 crate 是否已解决？        → 引依赖，不重复造轮子
4. 以上都不行                    → 最小范围 unsafe + 本章边界规则
```

### ER Item 16 联动

1. unsafe 放弃编译器自动校验 — **责任全在开发者**，失误 → UB  
2. **绝大多数**业务/高性能场景不需要手写 unsafe  
3. 必须用时：三条边界规则 + unsafe 隔离为实现细节  
4. 每处 `unsafe` 加 **`// SAFETY:`** — 列出满足的不变量  

---

## 四、工程落地流程

| 步骤 | 动作 |
|------|------|
| 1 | 列出所有须突破安全检查的底层操作 |
| 2 | 私有结构存裸指针/状态，对外屏蔽 |
| 3 | 修改不变量的代码收拢至**同一信任域** |
| 4 | 公共接口**前置校验**，再进极小 `unsafe {}` |
| 5 | 优先换标准库/第三方封装，削减手写量 |
| 6 | `// SAFETY:` + rustdoc + **Miri** 持续校验 |

---

## 五、核心总结

1. **缩小边界**：unsafe 私有，对外仅 Safe API。  
2. **信任域统一**：改 unsafe 状态的代码集中在同一模块。  
3. **优先复用**：避免手写 unsafe。  
4. **目标**：最小化人工审计范围，防止不变量泄露到外部 Safe 代码。  
5. **配套**：文档化前提 + Miri → [11](./11-documentation.md) · [12](./12-check-your-work.md)

---

## 速记

## 三大准则

1. **缩小边界** — private 字段 + safe 对外 API + 极小 `unsafe {}`  
2. **同一信任域** — 改不变量的代码全在一个 mod/impl 内  
3. **优先安全替代** — 标准库 / 成熟 crate；能不写 unsafe 就不写  

## 取舍流程

Safe 重构 → 标准库 → 社区 crate → 最后才手写 unsafe

## 不变量

指针有效 · len/cap · 初始化/Validity · 所有权 — **不得泄露到模块外**

## 配套

`// SAFETY:` · Miri · ER Item 16

## 自测

- [ ] 为何 `pub ptr: *mut T` 会破坏边界抽象？  
- [ ] 「信任域」与「缩小边界」差在哪？  
- [ ] 何时才值得手写 unsafe？

