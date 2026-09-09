# 02 · 字符设备：让 open/read/ioctl 落到你的代码

> **本节讲什么：** 内核里最基础、也是最能说明"用户态 syscall 如何落到驱动"的一类驱动。
> **对应动手：** [P5 · C1](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（GPIO 字符设备 + DTS + DT overlay，userspace 能 `open/read/ioctl`）

---

## 要点

| 概念 | 说明 |
|------|------|
| **主/次设备号** | 主号定位驱动，次号定位具体设备。`dev_t` = 主<<20 \| 次 |
| **`file_operations`** | 驱动实现的回调集合：`open`/`read`/`write`/`release`/`unlocked_ioctl`/`mmap`… |
| **设备节点** | `/dev/xxx` 由 `mknod` 或 udev/mdev 创建；节点本身只是"号码 + 名字"的映射 |
| **用户/内核数据边界** | 不能直接解引用用户指针！用 `copy_to_user` / `copy_from_user` |
| **`inode` vs `file`** | `inode` 是"这个设备"，`file` 是"这一次打开"（可有多次打开、各自 offset） |

---

## syscall → 驱动的完整路径

```
用户态 fd = open("/dev/foo", O_RDWR)
   ↓ glibc → syscall → 内核 sys_open
   ↓ VFS 查 inode 的 i_rdev → 主设备号 → 找到注册的 cdev
   ↓ cdev->ops->open(inode, filp)
返回 fd（内核里是 struct file *）
```

**这条链要能默出来** —— 它是"用户态/内核态"这个抽象的具体接缝。

---

## 骨架

```c
static dev_t devno;
static struct cdev my_cdev;
static struct class *my_class;

static int my_open(struct inode *inode, struct file *filp) { return 0; }
static ssize_t my_read(struct file *filp, char __user *buf,
                       size_t len, loff_t *off)
{
    /* 必须用 copy_to_user，不能 memcpy 到用户指针 */
    return 0;
}
static long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    /* cmd 用 _IOW/_IOR/_IOWR 构造，带 size 编码，天然防错 */
    return 0;
}

static const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .unlocked_ioctl = my_ioctl,
};
```

**现代写法：** 用 `alloc_chrdev_region` 动态申请设备号（别硬编码主号），配合 `device_create` 让 udev 自动建节点。

---

## HFT / 嵌入式关联

- **`ioctl` 是控制面**：数据面不该走 `read/write`（一次 syscall 一次拷贝）。HFT 里数据面走 `mmap` + 轮询（见 [06-dma-mmap](../06-dma-mmap/)）。
- **`copy_to_user` 的开销** 就是"用户态/内核态边界"的具象成本——这正是内核旁路技术要消灭的东西。

---

## 本节笔记

| # | 笔记 |
|---|------|
| 2.1 | [主次设备号与 dev_t](./2.1-majorminor-devt.md) |
| 2.2 | [**`open()` 如何落到驱动**（完整调用链）](./2.2-syscall-to-driver.md) |
| 2.3 | [用户/内核数据边界 · `copy_to_user`](./2.3-user-kernel-boundary.md) |
| 2.4 | [ioctl 命令编码 + 现代注册骨架](./2.4-ioctl-and-registration.md) |

---

## 验收

- [ ] 写过最小字符设备，`open/read/ioctl` 都能从用户态调用成功
- [ ] **能默出 `open()` 从 glibc 落到驱动 `open` 的完整路径**
- [ ] 用过 `copy_to_user`，能解释为什么不能直接解引用用户指针
- [ ] 用 `alloc_chrdev_region` 动态申请设备号，而非硬编码

---

## 衔接

- **上一步：** [01-hello-module](../01-hello-module/)
- **下一步：** [03-platform-dt](../03-platform-dt/)
- **卡住查书：** Madieu Ch4 · LDD3 Ch3、Ch6
