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



<details>
<summary>自测题（点击展开）</summary>

**Q1.** git bisect 如何定位回归 bug？

<details><summary>答案</summary>

1) `git bisect start` → 2) `git bisect bad <当前commit>` 标记有 bug → 3) `git bisect good <旧commit>` 标记无 bug → 4) git 自动 checkout 中间 commit → 5) 编译测试 → 6) `git bisect good/bad` → 7) 重复直到定位到引入 bug 的 commit。O(log n) 次编译。内核回归 bug 通常能在 10-15 次 bisect 内定位。

</details>

</details>
---
