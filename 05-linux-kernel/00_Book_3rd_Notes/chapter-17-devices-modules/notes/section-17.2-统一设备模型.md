## ② 统一设备模型 · The Device Model

**动机（2.6）：** 构建准确 **设备拓扑树** → **设备级电源管理**（例：关 USB 控制器前须先关 USB 鼠标）。

| 需求 | 统一表示设备 + 描述 **父子/总线** 关系 |

#### 核心组件

| 组件 | 角色 |
|------|------|
| **`kobject`** | **最核心** — 像 OOP **基类**；引用计数、名称、**父指针** → **层次结构** |
| **`ktype`** | 描述一族 kobject 的 **默认行为** — 析构、sysfs 操作、默认属性 |
| **`kset`** | **kobject 集合** — 容器（如「所有块设备」一组） |
| **`kref`** | **标准引用计数** — 用则增、完则减；**归零安全销毁** |

#### 嵌入式设计（同 list_head）

| 模式 | 说明 |
|------|------|
| `kobject` **嵌入** `cdev` 等 | 给驱动结构 **面向对象 + sysfs 生命周期** |

```
USB 控制器 kobject
    └── USB Hub kobject
            └── 鼠标 kobject    ← 关电须自底向上
```

→ **Ch 6** 嵌入结构 · **Ch 12** kref 与内存释放



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 设备模型的核心是什么？kobject/kset 的作用？

<details><summary>答案</summary>

设备模型用 kobject（内核对象基类）+ kset（对象集合）构建设备拓扑树。device → kobject 嵌入；bus → kset 管理同总线设备；driver → 注册到 bus。sysfs 是设备模型在用户态的投影。这套设计让电源管理、热插拔、设备发现可以自动化。HFT 网卡驱动也注册到设备模型中，ethtool/ip 通过 sysfs/netlink 操作。

</details>

</details>
---
