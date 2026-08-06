# ch04 Demo

```bash
make all
./demo01_sizeof
./demo03_subscript
./demo02_correct

# 错误 extern 演示（可能 SIGSEGV，慎用）
make demo02_wrong
./demo02_wrong
```

## demo02 跨文件

| 目标 | file2 声明 | 结果 |
|------|------------|------|
| demo02_correct | `extern char arr[]` | 正常输出 hello |
| demo02_wrong | `extern char *arr` | 可能崩溃或乱码 |

原理见 **4.2**、**4.4**。
