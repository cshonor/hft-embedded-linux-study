## 7.12 位置无关代码 (PIC)

### 为何需要 PIC

- 共享库 **加载地址不固定**（ASLR）— 代码段不能写死绝对地址
- **PIC** — 文本段可在任意基址执行；**数据引用** 通过 GOT/相对寻址

### 实现要点（x86-64）

- **PC 相对寻址** — `call`/`lea` 相对 `%rip`
- 全局变量访问常经 **GOT** — `@GOTPCREL`
- **`-fPIC`** 编译共享库；**`-fPIE`** 可执行文件（PIE = ET_DYN + 入口）

```bash
gcc -fPIC -shared -o libfoo.so foo.c
gcc -pie -o prog main.c -L. -lfoo
```

**HFT：**

- 生产二进制多 **PIE + ASLR**（安全）；极致延迟场景个别 **no-pie**（权衡安全，需合规）
- 共享 **行情解码 .so** 必须 PIC，否则重定位破坏代码共享

---

← [本章导读](../README.md)
