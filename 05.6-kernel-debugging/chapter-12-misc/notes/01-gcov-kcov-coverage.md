# GCOV / KCOV 代码覆盖率

> ⬜ 跳读 · Part 3: Diagnostics & Advanced Tools

## 概念详解

### GCOV (代码覆盖率)

GCOV 是 GCC 的代码覆盖率工具，统计哪些代码行被执行过，用于评估测试完整性。

```bash
# 内核配置
CONFIG_GCOV_KERNEL=y

# 启用特定目录的覆盖率
echo 1 > /sys/kernel/debug/gcov/path/to/module

# 覆盖率数据在 /sys/kernel/debug/gcov/
ls /sys/kernel/debug/gcov/
# kernel/  mm/  fs/  drivers/  ...

# 收集数据
cp -r /sys/kernel/debug/gcov/ /tmp/gcov_data/
lcov --capture --directory /tmp/gcov_data --output-file kernel.info
genhtml kernel.info --output-directory coverage_report

# 在浏览器中查看
# firefox coverage_report/index.html
```

### KCOV (内核代码覆盖率)

KCOV 是内核专用的覆盖率工具，专门为模糊测试设计——记录每次 syscall 执行的代码覆盖率。

```bash
# 内核配置
CONFIG_KCOV=y

# KCOV 主要用于 syzkaller 模糊测试
# 追踪单个系统调用的代码覆盖率
```

### GCOV vs KCOV

| 特性 | GCOV | KCOV |
|------|------|------|
| 目标 | 全局代码覆盖率 | 单次调用覆盖率 |
| 用途 | 评估测试完整性 | 模糊测试引导 |
| 粒度 | 代码行/分支 | 代码地址 |
| 收集方式 | 全局统计 | per-thread |
| 开销 | 中等 | 较低 |
| 主要用户 | 测试工程师 | syzkaller |

### GCOV 覆盖率报告

```
GCOV 报告示例:
  1:   10: void my_function(int x) {
  2:   10:     if (x > 0) {
  3:    8:         do_positive();
  4:    8:     } else {
  5:    2:         do_negative();
  6:   10:     }
  7:   10: }

说明:
  1:    执行次数
  10:   代码行号
  → 第3行执行了8次，第5行执行了2次
  → 两条路径都被覆盖
```

### KCOV 工作原理

```
KCOV 使用 __sanitizer_cov_trace_pc 插桩:
  每个基本块入口插入 coverage 收集点
  KCOV 在 per-thread buffer 中记录执行过的 PC 地址
  syzkaller 读取 buffer 获取覆盖率信息

流程:
  1. syzkaller 在测试进程中启用 KCOV
  2. 进程执行 syscall
  3. KCOV 记录执行过的内核代码地址
  4. syzkaller 收集覆盖率，指导后续测试生成
```

### HFT 关联应用

```bash
# HFT 模块测试覆盖率分析
# 1. 编译内核模块时启用 GCOV
make C=1 GCOV=1 modules

# 2. 在树莓派上运行测试
./run_hft_tests.sh

# 3. 收集覆盖率数据
cp -r /sys/kernel/debug/gcov/drivers/my_hft/ /tmp/gcov/
lcov --capture --directory /tmp/gcov/ --output-file hft.info
genhtml hft.info --output-directory hft_coverage

# 4. 分析覆盖率
# - 哪些代码路径被测试覆盖?
# - 哪些错误处理路径未覆盖?
# - 是否需要添加更多测试?
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** GCOV 和 KCOV 的区别？

> GCOV 追踪全局代码覆盖率（哪些代码被执行过），用于评估测试完整性。KCOV 追踪单个系统调用的覆盖率，用于模糊测试——syzkaller 用 KCOV 知道每个测试输入触发了哪些代码路径，指导后续生成。

**Q2:** KCOV 如何为 syzkaller 提供覆盖率引导？

> KCOV 在每个基本块入口插桩，记录执行过的 PC 地址到 per-thread buffer。syzkaller 读取 buffer 获取覆盖率信息，如果新输入覆盖了之前未执行的代码路径，保留并变异该输入，逐步探索新代码路径。

**Q3:** GCOV 报告中的数字代表什么？

> 左侧数字是该行代码被执行的次数。0 表示未执行（未覆盖）。通过覆盖率报告可以找出未测试的代码路径，特别是错误处理路径（通常难以触发）。

**Q4:** HFT 模块为什么要做覆盖率分析？

> 确保测试覆盖所有关键代码路径——特别是错误处理路径（如网络断开、内存不足、数据格式错误）。未覆盖的路径可能在生产环境中出问题。覆盖率分析帮助识别测试盲区。

**Q5:** GCOV 的性能开销如何？能用于生产环境吗？

> GCOV 有中等性能开销（每次基本块执行额外记录一次）。不适合生产环境。应在测试/staging 环境中运行测试用例并收集覆盖率数据。

</details>

## 交叉引用

- [05.6 ch12 syzkaller 模糊测试](../../chapter-12-misc/notes/03-syzkaller-fuzzing.md)
- [05.6 ch12 内核测试框架](../../chapter-12-misc/notes/02-kselftest-kunit.md)
