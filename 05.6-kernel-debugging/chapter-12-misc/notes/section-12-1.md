# 12.1 GCOV / KCOV 代码覆盖率

> ⬜ 跳读 · Part 3: Diagnostics & Advanced Tools

## 本节要点

### GCOV (代码覆盖率)

```bash
# 内核配置
CONFIG_GCOV_KERNEL=y

# 启用特定目录的覆盖率
# echo 1 > /sys/kernel/debug/gcov/path/to/module
# 覆盖率数据在 /sys/kernel/debug/gcov/

# 收集数据
cp -r /sys/kernel/debug/gcov/ /tmp/gcov_data/
lcov --capture --directory /tmp/gcov_data --output-file kernel.info
genhtml kernel.info --output-directory coverage_report
```

### KCOV (内核代码覆盖率，用于模糊测试)

```bash
# 内核配置
CONFIG_KCOV=y

# KCOV 主要用于 syzkaller 模糊测试
# 追踪单个系统调用的代码覆盖率
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** GCOV 和 KCOV 的区别？

> GCOV 追踪全局代码覆盖率（哪些代码被执行过），用于评估测试完整性。KCOV 追踪单个系统调用的覆盖率，用于模糊测试（syzkaller 用 KCOV 知道每个测试输入触发了哪些代码路径，指导后续生成）。

</details>
