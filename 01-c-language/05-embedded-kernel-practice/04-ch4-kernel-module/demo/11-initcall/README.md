# 11-initcall：initcall 机制用户态模拟实测

> **对应**：[4.11 initcall 机制全解](../../4.11-initcall/4.11-initcall机制全解.md)
> **环境**：gcc 13.3.0 / GNU ld 2.42 / x86-64
> **一键复现**：`cd 11-initcall && bash run.sh`

## 测试矩阵

| 编号 | 主题 | 关键结论 |
|---|---|---|
| T1 | `section` 属性放指针 | `readelf -S` 可见 `.initcall6.init` 段，`nm` 可见指针符号 |
| T2 | 链接脚本 + 循环执行 | `KEEP` + `__initcall_start/end` 导出，`for` 循环遍历执行 |
| T3 | 8 级别按序执行 | `pure → core → postcore → arch → subsys → fs → device → late` |
| T4 | `KEEP` vs gc-sections | **segfault 教学点**：`KEEP` 保留指针，但函数代码被 gc 删 → 调用时崩 |
| T5 | `alias` + `dlsym` | `init_module` 是 `my_driver_init` 的别名，`nm` 显示同地址 |
| T6 | `__init` 段释放 | `free_initmem` 后再调用 → BUG 警告（野指针） |
| T7 | 失败处理 | `ret<0` ERROR，`ret>0` WARNING，内核继续启动 |
| T8 | PREL32 vs 绝对地址 | 每项省 4 字节，1000 项省 4KB |
| T9 | 双版本对比 | 模块版 `alias`（全局符号 T）vs 内建版 `section`（局部指针 d） |
| T10 | 链接顺序 | 同级别内，`.o` 链接顺序决定执行顺序（a-b-c vs c-b-a） |

## 链接脚本

| 脚本 | 用途 |
|---|---|
| `initcall.lds` | 单级别（6）段收集，T2/T6/T7/T9/T10 用 |
| `initcall_levels.lds` | 8 级别段收集，T3 用 |
| `keep_test.lds` | KEEP() 测试，T4 用 |

## 运行

```bash
cd 11-initcall && bash run.sh
```

T4 的 gc-sections 版本会 segfault——这是教学点，不是 bug。`run.sh` 用 `|| true` 处理后继续。
