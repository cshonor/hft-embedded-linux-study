## 设备栈总览（与前面章节）

```
应用
  ├─ open("/dev/sdX")     ──► 块层（Ch 14）──► 驱动模块
  ├─ open("/dev/uio")     ──► 字符驱动 / UIO
  └─ socket()             ──► 网络子系统（非 /dev）
         ▲
    sysfs / uevent 暴露拓扑与配置
```

---
