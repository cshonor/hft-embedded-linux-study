# Item 23 · 逻辑脉络

← [Item 23 目录](./README.md)

```text
Item 21：Minor 加公开 API = Semver 兼容
         ↓
use external::* + cargo update → 新符号静默出现
         ↓
普通类型同名：本地定义优先 → 往往无害
Trait 方法同名：方法解析歧义 → 编译失败（E0034）
         ↓
对策：第三方 crate 显式 use；例外见 §3
```

---
