# P4 Part A — Hello World 模块 + 字符设备

> 从 `insmod hello.ko` 到注册一个能 `open/read/write` 的字符设备。
> **做法：项目驱动，[`05`](../../05-linux-kernel/) / [`05.5`](../../05.5-modern-kernel/) 笔记当字典。**

---

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [LKD 17.5 内核模块](../../05-linux-kernel/chapter-17-devices-modules/notes/section-17.5-内核模块.md) | 模块 = 可加载的 .ko，init/exit 入口 |
| [LKD 17.1 设备类型](../../05-linux-kernel/chapter-17-devices-modules/notes/section-17.1-设备类型.md) | 字符设备 = 流式访问，有 file_operations |
| [ULK 附录 B 模块](../../18-linux-kernel-deep/appendix-B-modules.md) | 模块加载/卸载的内核机制 |
| [09 Slab/kmalloc](../../06-linux-mm/chapter-08-slab-allocator/notes/section-4-尺寸缓存-与-kmalloc-kfree.md) | kmalloc = slab 分配器入口 |

---

## Phase 1：Hello World 模块（30 分钟）

### 做什么

最小内核模块：加载打印 "Hello"，卸载打印 "Goodbye"。

### 代码骨架

```c
// src/hello.c
#include <linux/module.h>
#include <linux/init.h>

static int __init hello_init(void) {
    pr_info("Hello, kernel!\n");
    return 0;
}

static void __exit hello_exit(void) {
    pr_info("Goodbye, kernel!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("P4 hello world");
```

```makefile
# Makefile
obj-m += hello.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
```

### 分步实现

1. **安装内核头**：`sudo apt install linux-headers-$(uname -r)`（WSL2 需自编译内核）
2. **写 hello.c + Makefile**
3. **`make`** → 生成 `hello.ko`
4. **`sudo insmod hello.ko`** → `dmesg | tail` 看到 "Hello, kernel!"
5. **`sudo rmmod hello`** → `dmesg | tail` 看到 "Goodbye, kernel!"

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 没装内核头 | `make` 报错找不到 KDIR | `apt install linux-headers-$(uname -r)` |
| `MODULE_LICENSE` 缺失 | 内核日志警告 "tainted" | 必须声明 GPL 才能用导出符号 |
| WSL2 默认内核没头 | 无法编译 | 需自编译 WSL2 内核或用树莓派 |
| `printk` 看不到 | `dmesg` 没输出 | 用 `pr_info` 替代；或调高日志级别 `pr_emerg` |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 模块加载机制 | [LKD 17.5](../../05-linux-kernel/chapter-17-devices-modules/notes/section-17.5-内核模块.md) |
| 模块的内核侧 | [ULK 附录 B](../../18-linux-kernel-deep/appendix-B-modules.md) |
| 现代 6.x 模块 API | [05.5 设备驱动](../../05.5-modern-kernel/chapter-08-device-driver-dt/) |

---

## Phase 2：字符设备注册（1 小时）

### 做什么

注册一个字符设备 `/dev/mydev`，用户态能 `open/read/write/close`。

### 代码骨架

```c
// src/chardev.c
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "mydev"
#define BUFFER_SIZE 4096

static dev_t dev_num;
static struct cdev my_cdev;
static char *kernel_buffer;

static int my_open(struct inode *inode, struct file *file) {
    pr_info("mydev: open\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buf,
                       size_t count, loff_t *offset) {
    size_t bytes_to_copy = min(count, (size_t)(BUFFER_SIZE - *offset));
    if (bytes_to_copy == 0) return 0;  // EOF
    if (copy_to_user(buf, kernel_buffer + *offset, bytes_to_copy))
        return -EFAULT;
    *offset += bytes_to_copy;
    return bytes_to_copy;
}

static ssize_t my_write(struct file *file, const char __user *buf,
                        size_t count, loff_t *offset) {
    size_t bytes_to_copy = min(count, (size_t)(BUFFER_SIZE - *offset));
    if (bytes_to_copy == 0) return -ENOSPC;
    if (copy_from_user(kernel_buffer + *offset, buf, bytes_to_copy))
        return -EFAULT;
    *offset += bytes_to_copy;
    return bytes_to_copy;
}

static int my_release(struct inode *inode, struct file *file) {
    pr_info("mydev: release\n");
    return 0;
}

static const struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .read    = my_read,
    .write   = my_write,
    .release = my_release,
};

static int __init chardev_init(void) {
    // 1. 分配设备号
    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);

    // 2. 初始化 cdev + 绑定 file_operations
    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;

    // 3. 添加 cdev 到内核
    cdev_add(&my_cdev, dev_num, 1);

    // 4. 分配内核缓冲区
    kernel_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!kernel_buffer) return -ENOMEM;

    pr_info("mydev: registered, major=%d minor=%d\n",
            MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit chardev_exit(void) {
    kfree(kernel_buffer);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("mydev: unregistered\n");
}

module_init(chardev_init);
module_exit(chardev_exit);
MODULE_LICENSE("GPL");
```

### 分步实现

1. **`alloc_chrdev_region`**：动态分配设备号（major + minor）
2. **`cdev_init` + `cdev_add`**：初始化字符设备结构 + 注册到内核
3. **`file_operations`**：绑定 open/read/write/release 回调
4. **`copy_to_user` / `copy_from_user`**：内核空间 ↔ 用户空间数据拷贝（不能用 `memcpy`！）
5. **创建设备节点**：`mknod /dev/mydev c <major> <minor>` 或用 `device_create` 自动创建
6. **用户态测试**：`echo "hello" > /dev/mydev` + `cat /dev/mydev`

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 用 `memcpy` 替代 `copy_to_user` | 随机崩溃/安全漏洞 | 用户态指针可能无效/不可写 |
| 忘了 `cdev_del` | 卸载后设备号泄漏 | exit 必须 `cdev_del` + `unregister_chrdev_region` |
| `offset` 没更新 | read 每次返回同样数据 | `*offset += bytes_to_copy` |
| `min()` 类型不匹配 | 编译警告 | 内核的 `min` 要求同类型，用 `min_t(size_t, ...)` |
| 缓冲区没检查边界 | 内核越界读写 → OOPS | read/write 都要检查 `*offset + count` |
| 没有 `mknod` | 用户态 open 报 ENOENT | 要么手动 mknod，要么 `device_create` |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 字符设备 / file_operations | [LKD 17.1](../../05-linux-kernel/chapter-17-devices-modules/notes/section-17.1-设备类型.md) |
| 内核内存分配 kmalloc | [09 Slab/kmalloc](../../06-linux-mm/chapter-08-slab-allocator/notes/section-4-尺寸缓存-与-kmalloc-kfree.md) |
| copy_to_user 原理 | [ULK ch09 地址空间](../../18-linux-kernel-deep/chapter-09-process-address-space/) |
| 现代 cdev API 变化 | [05.5 设备驱动](../../05.5-modern-kernel/chapter-08-device-driver-dt/) |

---

## Phase 3：用户态测试程序

### 代码骨架

```c
// src/user_test.c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    int fd = open("/dev/mydev", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    // 写
    const char *msg = "Hello from userspace!";
    write(fd, msg, strlen(msg));

    // 读
    lseek(fd, 0, SEEK_SET);  // 回到开头
    char buf[256] = {0};
    read(fd, buf, sizeof(buf));
    printf("Read back: %s\n", buf);

    close(fd);
    return 0;
}
```

### 测试

```bash
# 内核侧
make && sudo insmod chardev.ko
sudo mknod /dev/mydev c $(grep mydev /proc/devices | awk '{print $1}') 0
sudo chmod 666 /dev/mydev

# 用户侧
gcc -o user_test src/user_test.c
./user_test
# 预期输出：Read back: Hello from userspace!

# 清理
sudo rmmod chardev
sudo rm /dev/mydev
```

← [P4 索引](./README.md) · [07 模块](../../05-linux-kernel/)
