# 01 CSAPP · 动手代码

与 **01 模块章节笔记** 对照；读 CSAPP 时在本目录编译运行。

| 文件 | CSAPP | 笔记 |
|------|-------|------|
| [ch02-endian-and-padding-demo.c](./ch02-endian-and-padding-demo.c) | Ch2 §2.1.2–2.1.3 + padding 预告 | [§2.1.2](../chapter-02-representing-information/notes/section-2.1.2-数据大小与sizeof.md) · [§2.1.3](../chapter-02-representing-information/notes/section-2.1.3-寻址与字节序.md) · [ABI](../chapter-02-representing-information/notes/section-2.1.2-abi-application-binary-interface.md) |
| [pointer-and-bytes.c](./pointer-and-bytes.c) | Ch2 §2.1.3 endian 逐字节 | [§2.1.3](../chapter-02-representing-information/notes/section-2.1.3-寻址与字节序.md) |
| [pointer-stride-demo.c](./pointer-stride-demo.c) | Ch3 §3.8 指针步长 | [§3.8 指针步长详解](../chapter-03-machine-level-programs/notes/section-3.8-指针步长详解.md) |

```bash
cd 01-CSAPP-3rd/code
gcc -Wall -Wextra -std=c11 -o ch02_demo ch02-endian-and-padding-demo.c
gcc -Wall -Wextra -std=c11 -o pointer_bytes pointer-and-bytes.c
gcc -Wall -Wextra -std=c11 -o pointer_stride pointer-stride-demo.c
```

**02 C 语言模块** 只放 K&R / *Pointers on C* 索引；**CSAPP 概念与实验一律在本模块（01）**。
