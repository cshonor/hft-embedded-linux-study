# Ch7 demos

```bash
cc -Wall -Wextra -o sbrk_probe sbrk_probe.c && ./sbrk_probe
cc -Wall -Wextra -o free_and_sbrk free_and_sbrk.c && ./free_and_sbrk
# optional: numAllocs blockSize freeStep freeMin freeMax
./free_and_sbrk 1000 10240 1 1 999
```

| 文件 | 说明 |
|------|------|
| `sbrk_probe.c` | `sbrk(0)` 查询与扩/缩断点 |
| `free_and_sbrk.c` | Listing 7-1：`free` 后 break 常不降 |
