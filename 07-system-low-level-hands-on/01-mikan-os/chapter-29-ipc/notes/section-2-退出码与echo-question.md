## 2. 退出码与 echo $?

---

### 一、Ch21 遗留：强制打印 exit code

**早期：** 应用结束 **终端自动打印 `exit code: N`** — **管道/脚本场景烦人**。

---

### 二、隐式保存

```cpp
class Terminal {
    int last_exit_code_;
public:
    void OnAppExit(int code) {
        last_exit_code_ = code;
        // 不再默认 Print exit code
    }
};
```

**CallApp / exit syscall / KillApp** 路径均 **更新 last_exit_code_**。

→ [Ch21 exit](../chapter-21-window-apps/notes/section-4-exit系统调用与CallApp栈恢复.md)

---

### 三、echo $?

**内置命令扩展：**

```
> apps/rpn 2 3 +
5
> echo $?
0
> apps/nosuch
(app killed / not found)
> echo $?
1
```

| 语义 | 对齐 **POSIX / bash** |
|------|----------------------|
| **`$?`** | **上一命令** 退出码 |
| **管道后** | 通常为 **管道末段** 或 **按书规则** — 见 §4 **WaitFinish 取右端** |

**为管道准备：** 左端 **静默失败码** · 用户 **主动查询**。

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 管道](./section-3-管道机制与PipeDescriptor.md)
