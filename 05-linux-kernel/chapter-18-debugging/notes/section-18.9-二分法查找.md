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

#### 自动化：`git bisect run`

| 手法 | 说明 |
|------|------|
| `git bisect run ./test.sh` | 脚本 exit 0 = good、非 0 = bad——**全自动**走到定位 |
| 内核场景 | 脚本里：`make` → 装 → 跑复现负载 → 读结果；每轮 20 分钟 × 12 轮 ≈ 4 小时无人值守 |
| `git bisect log` / `git bisect replay` | 保存进度——中途机器崩了/换人接着 bisect |

```bash
#!/bin/sh
make -j$(nproc) || exit 125      # 编译失败=skip(125),别误标 good/bad
reboot_into_new_kernel || exit 125
run_repro && exit 0              # 复现负载通过=good
exit 1                           # 复现=bad
```

> `exit 125` 是 bisect 的 **skip 语义**——编译挂了/测试环境抖动的轮次跳过，不污染判定。

#### 数学与现实

| 项 | 值 |
|----|----|
| 复杂度 | O(log₂ n) 次测试：1 万个 commit → ~13 轮 |
| 前提 | **每个 commit 都可独立构建且行为确定**——内核 bisect 最大的现实成本是**环境抖动**（偶发 bug 复现率 90% 的机器会把二分树带歪） |
| 兜底 | 对概率性 bug：每轮**多次重复测试**取多数，或用 `git bisect skip` |

| 内核特有坑 | 对策 |
|------------|------|
| 中间 commit 编不过（依赖外部固件/编译器版本） | skip 语义（125） |
| config 变化（新代码要新 CONFIG） | `make oldconfig` 固定配置基线 |
| 回归其实在**用户态工具**（内核没错） | 先在旧内核+新工具交叉验证，别急着 bisect 内核 |

**HFT：** "性能回归"也能 bisect——测试脚本的判定从"功能对错"换成"延迟阈值"（如 p99 < X µs 为 good）。这是定位**哪次提交引入尾延迟劣化**的唯一系统化方法，与功能 bug 二分同构。

→ **Ch 2** `git clone` 内核树 · **Ch 20** 补丁流程



<details>
<summary>自测题（点击展开）</summary>

**Q1.** git bisect 如何定位回归 bug？

<details><summary>答案</summary>

1) `git bisect start` → 2) `git bisect bad <当前commit>` 标记有 bug → 3) `git bisect good <旧commit>` 标记无 bug → 4) git 自动 checkout 中间 commit → 5) 编译测试 → 6) `git bisect good/bad` → 7) 重复直到定位到引入 bug 的 commit。O(log n) 次编译。内核回归 bug 通常能在 10-15 次 bisect 内定位。

</details>

**Q2.** bisect 脚本里 `exit 125` 与 `exit 1` 有什么区别？为什么编译失败必须用 125？

<details><summary>答案</summary>

`exit 1` = 判定该 commit **有 bug**（bad）；`exit 125` = **跳过**（skip）该 commit，让 git 另选中点。编译失败的 commit 并非"有 bug"——把它标 bad 会把嫌疑区间整体带偏（真凶可能在被误标区间之外）。同理，测试环境抖动（复现负载偶发不触发）也该走 skip 或多次重试取多数：**二分的正确性完全依赖每轮判定的可信度**，一次误判就让后续所有收敛失去意义。

</details>

**Q3.** 概率性 bug（10 次里触发 7 次）怎么 bisect？

<details><summary>答案</summary>

不能单次判定——按 70% 触发率，单轮误判概率 30%，13 轮二分里几乎必有一次走错。做法：每轮**重复测试 N 次**（如 N=10，7+ 次触发才标 bad，0 次触发才标 good，中间态 skip），把单轮误判率压到可忽略（0.3^7 量级）；代价是每轮时间 ×N——所以概率 bug 的 bisect 要配自动化（`git bisect run`）+ 并行测试机。这是"二分的数学"与"工程预算"的典型折衷。

</details>

**Q4.** 内核 bisect 的测试脚本为什么要自己处理内核安装与重启？"当前运行的内核版本"与"当前源码版本"是什么关系？

<details><summary>答案</summary>

bisect 每轮 checkout 的是一个**源码版本**，而判定要在**编译并启动后的内核**上做——脚本必须完成 make → 安装（bzImage/initramfs 拷到 /boot、更新引导项）→ 重启 → 跑负载。运行时读 `uname -r`（配合本地版本号含 commit id，如 `6.6.0-bisect-g3f2a1b`）确认"现在跑的确实是本轮编译的内核"——**跳过这步验证是内核 bisect 最常见的翻车原因**（看似测了 13 轮，其实一直在测同一个旧内核）。

</details>

</details>
---
