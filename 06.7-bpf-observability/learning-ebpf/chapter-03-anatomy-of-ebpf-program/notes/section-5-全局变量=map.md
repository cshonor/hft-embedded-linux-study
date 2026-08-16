# 全局变量 = map

2019 年起支持全局变量，实现就是 map：
- `hello.bss`（array map，1 项）→ 初始化为 0 的全局变量（counter）
- `hello.rodata`（array map，1 项，frozen）→ 只读数据（格式串 "Hello World %d"）

有 BTF（`-g`）时 `bpftool map dump` 能漂亮打印变量名和值；没有则只能看裸十六进制（`19 01 00 00` = 小端 281）。

**pin 机制**：bpffs（`/sys/fs/bpf/`）上的伪文件持有程序/map 的引用；删除文件即释放（无 `prog unload` 命令时用 rm）。用户态程序退出后，pinned 的程序仍留在内核。
