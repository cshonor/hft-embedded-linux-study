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

```c
/* include/linux/kobject.h:64 — v6.6 原文 */
struct kobject {
	const char		*name;          /* 目录名 */
	struct list_head	entry;          /* 挂到 kset->list */
	struct kobject		*parent;        /* ← 层次结构就靠这一个指针 */
	struct kset		*kset;
	const struct kobj_type	*ktype;         /* 析构 + sysfs ops */
	struct kernfs_node	*sd;            /* sysfs 目录项（已 kernfs 化，见 17.3） */
	struct kref		kref;           /* 引用计数 */

	unsigned int state_initialized:1;
	unsigned int state_in_sysfs:1;
	unsigned int state_add_uevent_sent:1;
	unsigned int state_remove_uevent_sent:1;
	unsigned int uevent_suppress:1;
};
```

> **kobject 单独使用几乎没有意义**，它总是**被嵌入**到更大的结构里（`struct device`、`struct cdev`…）。
> 它的全部价值就是给宿主结构带来：**引用计数 + 层次结构 + sysfs 可见性 + 热插拔事件**这四件事。

---

#### 设备模型的三根支柱：bus / device / driver

```c
/* include/linux/device.h — v6.6 struct device（节选） */
struct device {
	struct kobject kobj;              /* ← 嵌入，见 Ch 6.1 */
	struct device		*parent;      /* 父设备（总线控制器） */
	struct device_private	*p;           /* 驱动核心私有数据（见下） */
	const char		*init_name;
	const struct bus_type	*bus;         /* 挂在哪条总线上 */
	struct device_driver	*driver;      /* 谁在驱动我 */
	void			*platform_data;
	void			*driver_data;
	u64			bus_dma_limit;    /* DMA 约束 */
	struct device_node	*of_node;     /* 设备树节点 */
	struct fwnode_handle	*fwnode;      /* 固件节点（ACPI / DT 统一抽象） */
	dev_t			devt;         /* 主次设备号 → 生成 sysfs 的 "dev" 文件 */
	const struct class	*class;       /* 功能分类（net / block / ...） */
	...
};

/* include/linux/device/driver.h:96 — v6.6 struct device_driver（节选） */
struct device_driver {
	const char		*name;
	const struct bus_type	*bus;
	struct module		*owner;
	bool suppress_bind_attrs;
	enum probe_type probe_type;              /* 同步/异步 probe 策略 */
	const struct of_device_id *of_match_table;/* 设备树匹配表 */
	int  (*probe)  (struct device *dev);
	int  (*remove) (struct device *dev);
	void (*shutdown)(struct device *dev);
	const struct attribute_group **dev_groups;/* 自动生成的 sysfs 属性 */
	const struct dev_pm_ops *pm;
};

/* include/linux/device/bus.h:80 — v6.6 struct bus_type（节选） */
struct bus_type {
	const char		*name;
	const struct attribute_group **bus_groups;
	const struct attribute_group **dev_groups;
	const struct attribute_group **drv_groups;
	int (*match)(struct device *dev, struct device_driver *drv);  /* ← 匹配规则 */
	int (*uevent)(const struct device *dev, struct kobj_uevent_env *env);
	int (*probe)(struct device *dev);
	...
};
```

```
               bus_type（如 pci_bus_type）
                        │
        ┌───────────────┴───────────────┐
        │                               │
   devices_kset                    drivers_kset
   （总线上的设备）                （能驱动它们的驱动）
        │                               │
        └────────► match() 配对 ◄────────┘
                        │
                        ▼
                     probe()
                  dev->driver = drv
```

**匹配是怎么发生的（三个入口，任一触发都会重新匹配）：**

| 触发 | 场景 |
|------|------|
| 新**设备**出现 | 热插拔、总线扫描 → 遍历该总线的所有驱动调 `match` |
| 新**驱动**注册 | `insmod` / `driver_register()` → 遍历该总线的所有设备调 `match` |
| **手动**触发 | 写 `/sys/bus/*/drivers_probe` 或 `bind`/`unbind` |

**`match()` 匹配的依据随时代变了：**

| 时代 | 匹配依据 | 说明 |
|------|---------|------|
| 早期 | 驱动**自报**支持的 ID 列表（`pci_device_id`） | 靠驱动自己列举 |
| 现代（**主流**） | **`of_match_table`**（设备树）/ **`acpi_match_table`** | 由**固件**描述"这块板子上有什么"，驱动声明"我能驱动什么样的节点" |

> 后者对应 `device->of_node` / `device->fwnode` 两个字段：`fwnode` 是设备树与 ACPI 的**统一抽象层**，
> 让驱动代码不必关心自己跑在 DT 系统（ARM/嵌入式）还是 ACPI 系统（x86 服务器）上。

---

#### device_private：驱动核心的"内部管线"

```c
/* drivers/base/base.h:108 — v6.6 原文 */
struct device_private {
	struct klist klist_children;      /* 子设备列表 */
	struct klist_node knode_parent;   /* 挂到父设备的 children */
	struct klist_node knode_driver;   /* 挂到驱动的 device 列表 */
	struct klist_node knode_bus;      /* 挂到总线的 device 列表 */
	struct klist_node knode_class;    /* 挂到 class 的列表 */
	struct list_head deferred_probe;  /* ← 延迟 probe 队列 */
	struct device_driver *async_driver;
	char *deferred_probe_reason;      /* ← 为什么延迟？可查！ */
	struct device *device;
	u8 dead:1;
};
```

**deferred probe（延迟探测）是现代设备模型的重要机制：**

```
驱动 A 的 probe 需要某个资源（如 regulator、clock、GPIO）
   ├─ 资源还没就绪（提供它的驱动 B 还没加载）
   ▼
返回 -EPROBE_DEFER → 设备进 deferred_probe 队列
   ├─ 之后每当有**新驱动注册成功**，内核重新尝试一遍这个队列
   ▼
驱动 B 加载完 → 队列重新跑 → A probe 成功
```

| 排障技巧 | |
|---------|--|
| 看延迟原因 | `cat /sys/bus/platform/drivers/.../.../` 或直接 `grep -r . /sys/kernel/debug/devices_deferred` |
| 驱动可声明 | `probe_type = PROBE_PREFER_ASYNCHRONOUS` 让慢设备的 probe **并行**进行，加快启动 |
| 反例 | `PROBE_FORCE_SYNCHRONOUS` 用于必须严格排序的设备（如某些存储控制器） |

```c
/* include/linux/device/driver.h:29 — v6.6 原文注释 */
 * @PROBE_PREFER_ASYNCHRONOUS: Drivers for "slow" devices which
 *	probing order is not essential for booting the system may
 *	...
 * @PROBE_DEFAULT_STRATEGY: Used by drivers that work equally well
 *	whether probed synchronously or asynchronously.
```

---

#### 嵌入式设计（同 list_head）

| 模式 | 说明 |
|------|------|
| `kobject` **嵌入** `cdev` 等 | 给驱动结构 **面向对象 + sysfs 生命周期** |

```
USB 控制器 kobject
    └── USB Hub kobject
            └── 鼠标 kobject    ← 关电须自底向上
```

> 电源管理**必须自底向上关、自顶向下开**——先停鼠标驱动，再停 Hub，最后停控制器。
> 设备模型提供了这棵树，**电源管理代码才第一次有可能正确实现**。这就是 2.6 引入统一设备模型的原始动机。

→ **Ch 6** 嵌入结构 · **Ch 12** kref 与内存释放 · [Ch 17.3 sysfs](./section-17.3-sysfs-虚拟文件系统.md) · [Ch 6.1 设计原则](../../chapter-06-kernel-data-structures/notes/section-6.1-设计原则.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Linux 设备模型的核心是什么？kobject/kset 的作用？

<details><summary>答案</summary>

设备模型用 kobject（内核对象基类）+ kset（对象集合）构建设备拓扑树。device → kobject 嵌入；bus → kset 管理同总线设备；driver → 注册到 bus。sysfs 是设备模型在用户态的投影。这套设计让电源管理、热插拔、设备发现可以自动化。HFT 网卡驱动也注册到设备模型中，ethtool/ip 通过 sysfs/netlink 操作。

</details>

**Q2.** 一个设备插入后，内核是怎么找到"该用哪个驱动"的？

<details><summary>答案</summary>

靠**总线（bus_type）做中介的 match() 回调**。设备和驱动都注册到同一条总线上，由总线的 `match()` 判断配对：

```c
/* include/linux/device/bus.h:80 */
struct bus_type {
	const char *name;
	int (*match)(struct device *dev, struct device_driver *drv);
	int (*uevent)(const struct device *dev, struct kobj_uevent_env *env);
	int (*probe)(struct device *dev);
	...
};
```

流程（`match` 成功 → 调 `probe` → 绑定 `dev->driver = drv`）：

1. **总线扫描**发现新设备 → `device_add()` → 遍历该总线的 `drivers_kset`，逐个调 `match`；
2. 或者**新驱动注册** → `driver_register()` → 遍历该总线的 `devices_kset`，逐个调 `match`；
3. `match` 成功 → 调用 `drv->probe(dev)` → probe 成功则绑定，**设备可用**；
4. probe 返回 `-EPROBE_DEFER`（依赖的资源还没就绪）→ 进 `deferred_probe` 队列，等下个驱动注册时重试。

**`match` 的依据随时代变了：**
- 早期：驱动自报 ID 列表（`pci_device_id`），靠驱动自己列举"我支持哪些设备"；
- 现代主流：**`of_match_table`（设备树）或 ACPI 表**——由固件描述"板子上有什么"，
  驱动声明"我能驱动什么样的节点"。对应 `struct device` 里的 `of_node` / `fwnode` 字段，
  其中 `fwnode` 是设备树与 ACPI 的**统一抽象层**，让同一份驱动代码能跑在 ARM 嵌入式和 x86 服务器上。

三个触发入口（任一都会触发重新匹配）：新设备出现、新驱动注册、手动写 `/sys/bus/*/.../bind`。

</details>

**Q3.** 驱动加载了，但设备没起来，`dmesg` 里也没报错，怎么查？

<details><summary>答案</summary>

最大嫌疑是 **deferred probe（延迟探测）**——probe 因为依赖未就绪返回了 `-EPROBE_DEFER`，设备被放进等待队列，这**不算错误，不会打日志**。

这是现代设备模型最常见的"静默失败"，查法：

```bash
# 直接看延迟探测队列（需要 debugfs）
cat /sys/kernel/debug/devices_deferred
# 输出示例：
# 0000:00:14.0  i2c_designware  supplier i2c-adapter not ready
```

`struct device_private`（`drivers/base/base.h:108`）里专门留了两个字段就是为了这个：
```c
struct list_head deferred_probe;
char *deferred_probe_reason;   /* ← 记录"为什么延迟"，上面那条信息就来自这里 */
```

**排查顺序：**
1. `cat /sys/kernel/debug/devices_deferred` —— 看是不是在等谁；
2. `lsmod` 确认依赖的驱动（regulator / clock / gpio / i2c 控制器）是否已加载。常见原因是**驱动编译成模块但没放进 initramfs**，加载顺序乱了；
3. 手动重放绑定：`echo 0000:00:14.0 > /sys/bus/pci/drivers/xxx/bind`，看这次报什么错；
4. 如果依赖确实无法自动解决（比如驱动 A 必须晚于驱动 B），用 **module softdep** 声明：
   `/etc/modprobe.d/xxx.conf` 里写 `softdep A pre: B`；
5. 想看 probe 的详细过程，开 `dyndbg`：`echo 'file drivers/base/dd.c +p' > /sys/kernel/debug/dynamic_debug/control`。

**HFT 相关场景**：把自研 FPGA/加速卡驱动编进内核时，如果它依赖某个 regulator 或 i2c，
一定要确认这些依赖**在 initramfs 里就位**，否则开机后设备静默不可用——重启一次就是几十秒的停机成本。

</details>

</details>
---
