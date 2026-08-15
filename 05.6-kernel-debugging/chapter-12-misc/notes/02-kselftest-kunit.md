# 内核测试框架 (kselftest / KUnit)

> ⬜ 跳读

## 概念详解

### KUnit (内核单元测试)

KUnit 是内核内建的单元测试框架，在内核中直接编写测试函数，不需要用户空间参与。

```c
#include <kunit/test.h>

static void test_my_function(struct kunit *test) {
    KUNIT_EXPECT_EQ(test, my_function(2), 4);
    KUNIT_EXPECT_NOT_NULL(test, my_ptr);
    KUNIT_EXPECT_TRUE(test, is_valid(my_data));
}

static void test_edge_case(struct kunit *test) {
    KUNIT_EXPECT_EQ(test, my_function(0), 0);
    KUNIT_EXPECT_EQ(test, my_function(-1), -1);
}

static struct kunit_case my_test_cases[] = {
    KUNIT_CASE(test_my_function),
    KUNIT_CASE(test_edge_case),
    {},
};

static struct kunit_suite my_suite = {
    .name = "my_module_tests",
    .test_cases = my_test_cases,
};

kunit_test_suite(my_suite);
```

### KUnit 断言宏

| 宏 | 功能 |
|------|------|
| `KUNIT_EXPECT_EQ(test, actual, expected)` | 期望相等 |
| `KUNIT_EXPECT_NE(test, actual, expected)` | 期望不等 |
| `KUNIT_EXPECT_TRUE(test, condition)` | 期望为真 |
| `KUNIT_EXPECT_FALSE(test, condition)` | 期望为假 |
| `KUNIT_EXPECT_NOT_NULL(test, ptr)` | 期望非 NULL |
| `KUNIT_EXPECT_NULL(test, ptr)` | 期望 NULL |
| `KUNIT_FAIL(test, message)` | 直接失败 |
| `KUNIT_ASSERT_*` | 断言版（失败则停止当前测试） |

### KUnit 运行

```bash
# 编译时运行 (QEMU)
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- kunit

# 指定测试套件
make ARCH=arm64 kunit CONFIG_KUNIT_MY_MODULE_TEST=y

# 在运行时运行
echo "my_module_tests" > /sys/kernel/debug/kunit/run
cat /sys/kernel/debug/kunit/results
```

### kselftest (内核自测)

kselftest 是用户空间测试框架，通过系统调用测试内核行为。

```bash
# 编译 kselftest
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- kselftest

# 运行特定测试
make ARCH=arm64 TARGETS=sched kselftest
make ARCH=arm64 TARGETS=memory-hotplug kselftest

# 运行所有测试
make ARCH=arm64 kselftest
```

### KUnit vs kselftest

| 特性 | KUnit | kselftest |
|------|-------|-----------|
| 测试位置 | 内核空间 | 用户空间 |
| 测试方式 | 直接调用内核函数 | 通过系统调用 |
| 适合 | 内部逻辑测试 | 用户接口测试 |
| 依赖 | 仅内核 | 用户空间工具 |
| 速度 | 快 | 较慢 |
| 真实性 | 单元级 | 系统级 |

### HFT 关联应用

```c
// HFT 模块的 KUnit 测试示例
#include <kunit/test.h>

static void test_order_book_insert(struct kunit *test) {
    struct order_book *book = create_order_book();
    
    // 测试插入买单
    insert_order(book, &(struct order){.price=100, .qty=10, .side=BID});
    KUNIT_EXPECT_EQ(test, book->bid_count, 1);
    KUNIT_EXPECT_EQ(test, best_bid(book), 100);
    
    // 测试插入更高的买单
    insert_order(book, &(struct order){.price=101, .qty=5, .side=BID});
    KUNIT_EXPECT_EQ(test, book->bid_count, 2);
    KUNIT_EXPECT_EQ(test, best_bid(book), 101);  // 最高价在前
    
    free_order_book(book);
}

static void test_order_matching(struct kunit *test) {
    struct order_book *book = create_order_book();
    
    insert_order(book, &(struct order){.price=100, .qty=10, .side=BID});
    insert_order(book, &(struct order){.price=100, .qty=5, .side=ASK});
    
    // 100 价位的买单和卖单应该匹配
    KUNIT_EXPECT_EQ(test, check_match(book), 1);
    KUNIT_EXPECT_EQ(test, book->bid_count, 0);  // 匹配后清除
    
    free_order_book(book);
}
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KUnit 和 kselftest 的区别？

> KUnit 是**单元测试**框架——在内核中编写测试函数，测试内部函数的行为，不需要用户空间参与。kselftest 是**系统测试**框架——在用户空间运行测试程序，通过系统调用测试内核行为。KUnit 适合测试内部逻辑，kselftest 适合测试用户可见接口。

**Q2:** KUnit 的 EXPECT 和 ASSERT 有什么区别？

> `KUNIT_EXPECT_*` 失败后继续执行当前测试函数（记录失败但继续检查其他断言）。`KUNIT_ASSERT_*` 失败后立即停止当前测试函数（后续代码不再执行）。EXPECT 适合多个独立检查，ASSERT 适合前置条件检查。

**Q3:** HFT 模块为什么应该写 KUnit 测试？

> (1) 确保核心逻辑正确（如订单匹配、价格计算）；(2) 回归测试——修改代码后快速验证未破坏功能；(3) 边界条件测试（如空订单簿、最大数量）；(4) CI 中自动运行，防止引入 bug。

**Q4:** kselftest 适合测试什么类型的内核功能？

> 适合测试用户可见的接口——如系统调用行为、/proc 文件内容、设备 ioctl 等。kselftest 从用户空间发起操作，验证内核的响应是否正确。不适合测试内核内部函数（用 KUnit）。

**Q5:** KUnit 测试可以在生产环境运行吗？

> 可以。KUnit 测试编译为模块，可以按需加载/卸载。测试在内核空间运行，开销可控。但建议在 staging 环境运行，避免测试代码影响生产性能。

</details>

## 交叉引用

- [05.6 ch12 GCOV/KCOV 代码覆盖率](../../chapter-12-misc/notes/01-gcov-kcov-coverage.md)
- [05.6 ch12 syzkaller 模糊测试](../../chapter-12-misc/notes/03-syzkaller-fuzzing.md)
