# Item 25 · 核心知识点

← [Item 25 目录](./README.md)

### 依赖图（Dependency graph）

- 程序 → 直接依赖 → 传递依赖 → 形成**依赖树/图**。

### crates.io 扁平命名空间

- Crate **名字**与 **feature 名字**共享同一命名空间（同一 crate 内）。

### 多版本共存

- Cargo 可在同一二进制图中链接同一 crate 的多个 **semver-incompatible** 版本（如 `1.2` + `3.1`）。
- **兼容**版本（如 `1.2` + `1.3`）→ 解析为**一个**最高兼容版（如 `1.3`）。

### Feature 统一（Feature unification）

- 图中多处依赖**同一版本**的 crate，但各自启用不同 features → Cargo 取**并集**，用合并后的 feature 集**只构建一次**。

---
