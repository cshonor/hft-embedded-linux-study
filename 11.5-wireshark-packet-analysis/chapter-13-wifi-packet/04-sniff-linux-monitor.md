# 13.4 在 Linux 上嗅探无线网络

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · CLI：[第6章](../chapter-06-tshark-tcpdump/chapter-summary.md)

**核心主旨**：`iw` / `iwconfig` 启 Monitor、切信道；配合 Wireshark 或 `tcpdump`。

## 核心知识点

### 典型命令（教材逻辑）

```bash
# 监听模式（接口名按实际，如 wlan0）
sudo iwconfig wlan0 mode monitor
# 或新式：
sudo ip link set wlan0 down
sudo iw dev wlan0 set type monitor
sudo ip link set wlan0 up

# 切换信道
sudo iwconfig wlan0 channel 3
# 或：
sudo iw dev wlan0 set channel 3
```

| 场景 | 做法 |
|------|------|
| 抓 AP 流量 | 信道设为与 AP **相同** |
| 扫多信道 | 脚本循环 channel，分段 pcap 再合并 |

```bash
sudo tcpdump -i wlan0 -w ch6.pcap
sudo wireshark -i wlan0   # 或 dumpcap
```

## 抓包/实操记录

| 检查 | 命令 |
|------|------|
| 模式 | `iw dev wlan0 info` → type monitor |
| 信道 | `iw dev wlan0 info` → channel |

## 疑问与总结

- NetworkManager 可能抢回 managed；抓包时停 NM 或专用接口。
- 5GHz 信道编号与 2.4GHz 不同，radiotap 中核对 `wlan_radio.channel`。
