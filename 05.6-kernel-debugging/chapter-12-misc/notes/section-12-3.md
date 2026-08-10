# 12.3 syzkaller 模糊测试

> ⬜ 跳读

## 本节要点

### syzkaller 概述

syzkaller 是 Google 开发的内核模糊测试工具，自动生成随机系统调用序列，发现内核崩溃。

### 工作流程

```
syzkaller
  → 生成随机 syscall 序列
  → 在 QEMU 中执行
  → KCOV 收集覆盖率
  → 如果崩溃: 保存日志 + 最小化复现
  → 基于覆盖率指导生成新序列
  → 循环
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
        "count": 4,
        "cpu": 2,
        "mem": 2048,
    }
}

# 运行
syz-manager -config syz.cfg
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** syzkaller 如何知道哪些代码路径已被覆盖？

> 通过 KCOV。syzkaller 在每个测试进程中启用 KCOV，KCOV 记录该进程执行过的所有内核代码地址。syzkaller 收集这些地址，构建覆盖率图。基于覆盖率指导生成新的测试输入——优先探索未被覆盖的代码路径。


**Q:** syzkaller 如何利用 KCOV 做覆盖率引导的模糊测试？

> syzkaller 生成随机 syscall 序列（基于 syscall 描述模板），执行后通过 KCOV 获取覆盖率。如果新输入覆盖了之前未执行的代码路径，syzkaller 保留该输入并变异它（类似遗传算法）。这样逐步探索新代码路径，比纯随机高效得多。

**Q:** syzkaller 发现的内核 bug 通常是什么类型？

> 主要是：(1) KASAN 检测的内存 bug（UAF/OOB）；(2) LOCKDEP 检测的锁问题；(3) WARN/panic 触发的逻辑错误；(4) KMSAN 检测的未初始化使用。syzkaller 在 Linux 内核社区发现了数百个 bug，是最重要的自动化测试工具之一。

</details>

## 交叉引用

- [05.6 ch05 KASAN](chapter-05-memory-debug-1/notes/section-5-2.md)
- [05.6 ch08 LOCKDEP](chapter-08-lock-debug/notes/section-8-2.md)
