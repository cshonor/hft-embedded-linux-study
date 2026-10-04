#!/bin/bash
# fio-seqread.sh — fio 顺序读基准（ch08 §8.7-8.8 块4）
# 用法: ./fio-seqread.sh [测试文件] [大小]
# 出自: notes/section-8.7-8.8-实验与调优.md
set -u

FILE="${1:-/data/testfile}"
SIZE="${2:-32G}"

command -v fio >/dev/null || { echo "未安装: sudo apt install fio"; exit 1; }
DIR=$(dirname "$FILE")
[ -w "$DIR" ] || { echo "目录不可写: $DIR（换 ./fio-seqread.sh /tmp/testfile 1G）"; exit 1; }

echo "fio 顺序读: $FILE ($SIZE, direct=1 绕过页缓存测真实磁盘)"
fio --name=seqread --filename="$FILE" --size="$SIZE" \
    --rw=read --bs=1M --direct=1 --ioengine=libaio \
    --runtime=60 --time_based --group_reporting \
    --percentile_list=50:99:99.9

echo
echo "对照实验建议："
echo "  --direct=0（走页缓存，测缓存命中路径）"
echo "  --bs=4k（小块随机读 --rw=randread，测 IOPS 而不是带宽）"
