# Item 30 · 核心知识点

← [Item 30 目录](./README.md)

| 类型 | 位置 / 触发 | 可见性 | 用途 |
|------|-------------|--------|------|
| **单元测试** | 同文件 `#[cfg(test)] mod tests` | 可测 **私有** | 白盒、小块逻辑 |
| **集成测试** | 根目录 **`tests/`** | 仅 **pub API** | 黑盒、模块协作 |
| **Doc tests** | `///` 里 code block | pub API | 文档与行为同步 |
| **Examples** | **`examples/`** | pub API | 用户向完整用法 |
| **Benchmarks** | `benches/` 或 bench crate | — | 性能回归 |
| **Fuzz** | **`cargo-fuzz`** (libFuzzer) | — | 随机输入找 crash |

---
