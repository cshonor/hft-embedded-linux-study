# Part B — 显式空闲链表 malloc

空闲块 payload 开头存 next/prev，malloc 只扫空闲链。64 位最小空闲块 32 字节。分离适配见 [../Part-B-malloc.md](../Part-B-malloc.md)，还没做。

```bash
make test
```
