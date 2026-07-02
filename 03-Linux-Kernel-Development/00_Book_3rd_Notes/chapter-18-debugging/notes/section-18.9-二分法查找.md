## ⑧ 二分法查找 · `git bisect`

| 场景 | 当前版本有 bug，不知 **哪次提交** 引入 |
|------|----------------------------------------|
| 方法 | 找 **已知好** 与 **已知坏** commit → **二分测试** |

```bash
git bisect start
git bisect bad          # 当前坏
git bisect good v4.19   # 已知好标签
# 反复：编译/测试 → git bisect good|bad
git bisect reset
```

→ **Ch 2** `git clone` 内核树 · **Ch 20** 补丁流程

---
