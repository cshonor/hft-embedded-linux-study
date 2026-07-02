## 6. cp 命令与小结

---

### 一、cp 实现

```cpp
int main(int argc, char** argv) {
    int src = open(argv[1], O_RDONLY);
    int dst = open(argv[2], O_WRONLY | O_CREAT);
    char buf[512];
    ssize_t n;
    while ((n = read(src, buf, sizeof buf)) > 0)
        write(dst, buf, n);
    close(src);
    close(dst);
    return 0;
}
```

```
> cp readme.txt copy.txt
> cat copy.txt
```

| 验证 | 链 |
|------|-----|
| **读源** | Ch25 **Read · 簇链** |
| **建目标** | **O_CREAT** |
| **写目标** | **Write · AllocateClusterChain** |
| **纯 stdio** | **open/read/write/close** |

---

### 二、与 grep + @stdin 组合

```
> grep hello @stdin
hello world
^D
hello world
```

**读 stdin · 写 stdout** — **过滤器** 模式 — **Shell 管道前奏**（Ch29 IPC 可 **真管道**）。

---

### 三、本章总结

| 成果 | 说明 |
|------|------|
| **fd 继承** | **Terminal / FAT 多态** |
| **0/1/2** | **stdin/stdout/stderr** |
| **Echo · @stdin · Ctrl+D** | **键盘字节流 + EOF** |
| **O_CREAT · ExtendCluster** | **新建/扩目录** |
| **Write** | **分配簇 · 写盘** |
| **cp** | **端到端复制** |

```
Ch26 统一 I/O
    ↓
Ch27 应用内存管理
Ch29 IPC · 管道
```

---

### 四、后续索引

| Ch26 主题 | 继续读 |
|----------|--------|
| 应用内存 | [chapter-27-app-memory](../chapter-27-app-memory/) ⚪ |
| 读文件 | [chapter-25-app-read-file](../chapter-25-app-read-file/) 🟡 |
| IPC | [chapter-29-ipc](../chapter-29-ipc/) |
| TLPI | [06-The-Linux-Programming-Interface](../../../06-The-Linux-Programming-Interface/) |

---

← [5. Write](./section-5-Write与标准输出.md) · [Ch 25](../chapter-25-app-read-file/) · [Ch 26 导读](../README.md)
