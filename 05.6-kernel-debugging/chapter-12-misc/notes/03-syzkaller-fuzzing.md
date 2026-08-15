# syzkaller 模糊测试

> ⬜ 跳读

## 概念详解

### syzkaller 概述

syzkaller 是 Google 开发的内核模糊测试工具，自动生成随机系统调用序列，发现内核崩溃。它利用 KCOV 做覆盖率引导，比纯随机测试高效得多。

### 工作流程

```
syzkaller 工作流程:
  1. 生成随机 syscall 序列（基于 syscall 描述模板）
  2. 在 QEMU 中执行
  3. KCOV 收集覆盖率
  4. 如果崩溃: 保存日志 + 最小化复现
  5. 基于覆盖率指导生成新序列
  6. 优先探索未覆盖的代码路径
  7. 循环

覆盖率引导:
  - 新输入覆盖了新代码路径 → 保留并变异
  - 新输入未覆盖新代码路径 → 丢弃
  - 类似遗传算法: 保留"好的"输入，变异生成新输入
```

### 基本配置

```bash
# 安装
go install github.com/google/syzkaller/cmd/syz-manager@latest

# 配置文件 syz.cfg
{
    "target": "linux/arm64",
    "http": "127.0.0.1:56741",
    "workdir": "/syzkaller/workdir",
    "syzkaller": "/syzkaller",
    "image": "/qemu/disk.img",
    "kernel_obj": "/linux-source",
    "type": "qemu",
    "vm": {
        "count": 4,       // 4 个 QEMU 实例并行
        "cpu": 2,          // 每个 VM 2 个 CPU
        "mem": 2048,       // 每个 VM 2GB 内存
    }
}

# 运行
syz-manager -config syz.cfg
# 浏览器访问 http://127.0.0.1:56741 查看进度
```

### syzkaller 发现的 bug 类型

| bug 类型 | 检测工具 | 说明 |
|---------|---------|------|
| 内存越界 | KASAN | UAF/OOB/双重释放 |
| 数据竞争 | KCSAN | 无锁并发访问 |
| 锁问题 | LOCKDEP | 死锁/锁序错误 |
| 逻辑错误 | WARN/panic | BUG_ON 断言失败 |
| 未初始化使用 | KMSAN | 使用未初始化变量 |

### syzkaller 的 syscall 描述

```go
// syzkaller 用自定义 DSL 描述 syscall 接口
// 示例: 描述 open() 系统调用
open(filename ptr[in, filename], flags flags[open_flags], mode flags[open_mode]) fd

// 描述 ioctl() 接口
ioctl(fd fd, cmd const[MY_IOCTL], arg ptr[in, my_struct])

// 描述自定义设备接口
syz_open_dev(dev ptr[in, string["/dev/my_device"]], flags flags[open_flags], mode flags[open_mode]) fd
```

### HFT 关联应用

```bash
# HFT 模块的 syzkaller 模糊测试
# 1. 编写 syscall 描述
#    描述 HFT 模块的 ioctl 接口

# 2. 编译带 KCOV + KASAN + LOCKDEP 的内核
#    CONFIG_KCOV=y
#    CONFIG_KASAN=y
#    CONFIG_LOCKDEP=y

# 3. 在 QEMU 中运行 syzkaller
#    syz-manager -config hft_syz.cfg

# 4. 分析发现的 bug
#    syzkaller 自动保存崩溃日志和复现脚本
#    crash-log.txt + repro.c

# 5. 修复并验证
```

### syzkaller Web 界面

```
http://127.0.0.1:56741
  Dashboard:
    - 覆盖率统计
    - 发现的 bug 列表
    - 当前执行的测试
    - 每个 VM 的状态
    - 崩溃复现脚本
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** syzkaller 如何知道哪些代码路径已被覆盖？

> 通过 KCOV。syzkaller 在每个测试进程中启用 KCOV，KCOV 记录该进程执行过的所有内核代码地址。syzkaller 收集这些地址，构建覆盖率图。基于覆盖率指导生成新的测试输入——优先探索未被覆盖的代码路径。

**Q2:** syzkaller 如何利用 KCOV 做覆盖率引导的模糊测试？

> syzkaller 生成随机 syscall 序列，执行后通过 KCOV 获取覆盖率。如果新输入覆盖了之前未执行的代码路径，syzkaller 保留该输入并变异它。这样逐步探索新代码路径，比纯随机高效得多。

**Q3:** syzkaller 发现的内核 bug 通常是什么类型？

> 主要是：(1) KASAN 检测的内存 bug（UAF/OOB）；(2) LOCKDEP 检测的锁问题；(3) WARN/panic 触发的逻辑错误；(4) KMSAN 检测的未初始化使用。syzkaller 在 Linux 内核社区发现了数百个 bug。

**Q4:** HFT 模块如何用 syzkaller 做模糊测试？

> (1) 编写 HFT 模块 ioctl 接口的 syscall 描述；(2) 编译带 KCOV+KASAN+LOCKDEP 的内核；(3) 在 QEMU 中运行 syzkaller；(4) 分析发现的 bug 并修复。模糊测试能发现手工测试难以覆盖的边界条件。

**Q5:** syzkaller 为什么需要多个 QEMU 实例？

> 并行测试提高效率——每个 VM 独立运行不同的测试输入。多 VM 可以同时探索不同的代码路径，加快覆盖率增长。syz-manager 管理所有 VM 的协调和结果收集。

</details>

## 交叉引用

- [05.6 ch12 GCOV/KCOV 代码覆盖率](../../chapter-12-misc/notes/01-gcov-kcov-coverage.md)
- [05.6 ch12 内核测试框架](../../chapter-12-misc/notes/02-kselftest-kunit.md)
- [05.6 ch05 KASAN](../../chapter-05-memory-debug-1/notes/02-kasan.md)
- [05.6 ch08 LOCKDEP](../../chapter-08-lock-debug/notes/02-lockdep.md)
