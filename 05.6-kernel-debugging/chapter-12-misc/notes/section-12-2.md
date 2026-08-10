# 12.2 内核测试框架 (kselftest / KUnit)

> ⬜ 跳读

## 本节要点

### KUnit (内核单元测试)

```c
#include <kunit/test.h>

static void test_my_function(struct kunit *test) {
    KUNIT_EXPECT_EQ(test, my_function(2), 4);
    KUNIT_EXPECT_NOT_NULL(test, my_ptr);
}

static struct kunit_case my_test_cases[] = {
    KUNIT_CASE(test_my_function),
    {},
};

static struct kunit_suite my_suite = {
    .name = "my_module_tests",
    .test_cases = my_test_cases,
};

kunit_test_suite(my_suite);
```

```bash
# 运行 KUnit 测试
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- kunit

# 或在运行时
echo "my_module_tests" > /sys/kernel/debug/kunit/run
cat /sys/kernel/debug/kunit/results
```

### kselftest (内核自测)

```bash
# 编译 kselftest
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- kselftest

# 运行特定测试
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- TARGETS=sched kselftest
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KUnit 和 kselftest 的区别？

> KUnit 是**单元测试**框架——在内核中编写测试函数，测试内部函数的行为，不需要用户空间参与。kselftest 是**系统测试**框架——在用户空间运行测试程序，通过系统调用测试内核行为。KUnit 适合测试内部逻辑，kselftest 适合测试用户可见接口。


**Q:** kselftest 和 KUnit 的区别？

> kselftest：用户态测试框架，从用户空间发起 syscall 测试内核行为（如 fork/exit/mmap）。KUnit：内核内建单元测试框架，在内核中直接测试函数（不需要用户态参与）。kselftest 适合系统级测试，KUnit 适合函数级测试。

</details>

## 交叉引用

- [05.6 ch12 syzkaller](chapter-12-misc/notes/section-12-3.md)
