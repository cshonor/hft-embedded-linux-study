# Item 35 · 易错细节

← [Item 35 目录](./README.md)

| 陷阱 | 说明 |
|------|------|
| **手写 struct 字段顺序/类型错** | 编译链接**仍成功** → 内存错乱 |
| **改 C API 未重跑 bindgen** | 签入旧 `generated.rs` → CI diff 应拦截 |
| **把 bindgen 输出当 safe 用** | `-sys` 层必须再包一层 |
| **C++ 大项目硬 bindgen** | 模板/重载/命名空间 → 用 **cxx** |

---
