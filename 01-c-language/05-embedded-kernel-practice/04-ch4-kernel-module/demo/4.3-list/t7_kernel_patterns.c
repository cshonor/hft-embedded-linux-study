/* T7: 内核真实用例模拟 —— file_operations / 设备链表 / private_data
 *
 * 模拟内核里三种典型用法:
 * 1. 全局设备链表 (list_head 挂所有设备)
 * 2. 从通用指针取回 private_data (container_of)
 * 3. 按条件查找节点 (list_for_each_entry + break)
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>

struct list_head { struct list_head *next, *prev; };
static inline void INIT_LIST_HEAD(struct list_head *list) { list->next = list; list->prev = list; }
static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
    next->prev = new; new->next = next; new->prev = prev; prev->next = new;
}
static inline void list_add_tail(struct list_head *new, struct list_head *head) { __list_add(new, head->prev, head); }

#define container_of(ptr, type, member) ({ \
    const __typeof__(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })
#define list_entry(ptr, type, member) container_of(ptr, type, member)
#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, __typeof__(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, __typeof__(*pos), member))

/* 模拟 file_operations (简化版) */
struct file_operations {
    int (*open)(struct file *);
    int (*read)(struct file *, char *, int);
    int (*close)(struct file *);
};

struct file {
    int f_mode;
    void *private_data;   /* 关键: 指向 mydrv_priv */
};

/* 驱动私有数据 */
struct mydrv_priv {
    int device_id;
    char bus_name[8];
    struct file_operations fops;  /* 内嵌 fops */
    struct list_head dev_list;    /* 挂在全局设备链表 */
    int is_open;
};

/* 全局设备链表 */
static struct list_head all_devices;

/* open 回调: 从 file->private_data 取回 priv */
static int my_open(struct file *f)
{
    /* 内核常见写法: container_of(f->private_data, ...) */
    struct mydrv_priv *priv = f->private_data;
    priv->is_open = 1;
    printf("  open: device_id=%d bus=%s\n", priv->device_id, priv->bus_name);
    return 0;
}

static int my_read(struct file *f, char *buf, int len)
{
    struct mydrv_priv *priv = f->private_data;
    printf("  read: device_id=%d len=%d\n", priv->device_id, len);
    return len;
}

static int my_close(struct file *f)
{
    struct mydrv_priv *priv = f->private_data;
    priv->is_open = 0;
    printf("  close: device_id=%d\n", priv->device_id);
    return 0;
}

/* 按 id 查找设备 */
struct mydrv_priv *find_device_by_id(int id)
{
    struct mydrv_priv *pos;
    list_for_each_entry(pos, &all_devices, dev_list) {
        if (pos->device_id == id)
            return pos;
    }
    return NULL;
}

int main(void)
{
    printf("=== T7: 内核真实用例模拟 ===\n\n");

    INIT_LIST_HEAD(&all_devices);

    /* 注册三个设备 */
    static struct mydrv_priv dev0 = {
        .device_id = 0,
        .bus_name = "spi",
        .fops = { .open = my_open, .read = my_read, .close = my_close },
        .dev_list = {},
    };
    static struct mydrv_priv dev1 = {
        .device_id = 1,
        .bus_name = "i2c",
        .fops = { .open = my_open, .read = my_read, .close = my_close },
        .dev_list = {},
    };
    static struct mydrv_priv dev2 = {
        .device_id = 2,
        .bus_name = "uart",
        .fops = { .open = my_open, .read = my_read, .close = my_close },
        .dev_list = {},
    };
    INIT_LIST_HEAD(&dev0.dev_list);
    INIT_LIST_HEAD(&dev1.dev_list);
    INIT_LIST_HEAD(&dev2.dev_list);
    list_add_tail(&dev0.dev_list, &all_devices);
    list_add_tail(&dev1.dev_list, &all_devices);
    list_add_tail(&dev2.dev_list, &all_devices);

    printf("--- 全局设备链表 ---\n");
    struct mydrv_priv *pos;
    list_for_each_entry(pos, &all_devices, dev_list) {
        printf("  device_id=%d bus=%s fops=%p dev_list=%p\n",
               pos->device_id, pos->bus_name, (void*)&pos->fops, (void*)&pos->dev_list);
    }

    printf("\n--- 按 id 查找 (找 id=1) ---\n");
    struct mydrv_priv *found = find_device_by_id(1);
    if (found) {
        printf("  found: device_id=%d bus=%s\n", found->device_id, found->bus_name);
    }

    printf("\n--- 模拟 open/read/close ---\n");
    struct file f = { .f_mode = 1, .private_data = found };
    found->fops.open(&f);
    found->fops.read(&f, NULL, 100);
    found->fops.close(&f);

    printf("\n--- 从 fops 取回 priv (container_of 的另一用法) ---\n");
    /* 内核里也有从某个内嵌成员反推宿主的场景 */
    struct mydrv_priv *via_fops = container_of(&found->fops, struct mydrv_priv, fops);
    printf("  via_fops = %p (== found? %s)\n",
           (void*)via_fops, via_fops == found ? "yes" : "NO");
    printf("  -> fops 在 priv 内部的偏移 = %zu\n", offsetof(struct mydrv_priv, fops));

    printf("\n--- 找不存在的 id=99 ---\n");
    struct mydrv_priv *notfound = find_device_by_id(99);
    printf("  find_device_by_id(99) = %p\n", (void*)notfound);
    return 0;
}
