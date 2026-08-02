# Ch49 demos — mmap

在 Linux / WSL 下于本目录执行：

```bash
gcc -Wall -Wextra -o mmap_anon mmap_anon.c
./mmap_anon

gcc -Wall -Wextra -o mmap_file mmap_file.c
./mmap_file /tmp/ch49-mmap.dat
```

| 文件 | 说明 |
|------|------|
| `mmap_anon.c` | `MAP_SHARED\|MAP_ANONYMOUS` 父子 IPC |
| `mmap_file.c` | `MAP_SHARED` 文件映射；父写子读 + `msync` |
