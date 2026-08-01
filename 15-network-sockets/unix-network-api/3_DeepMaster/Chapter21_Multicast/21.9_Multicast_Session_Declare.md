# 21.9 接收 IP 多播基础设施会话声明

---

## 案例：SAP / SDP（MBone）

音视频会议会话经 **SAP** + **SDP** 多播宣告。

---

## 接收步骤

```text
1. socket(UDP)
2. setsockopt(SO_REUSEADDR)   /* bind 前，多进程同端口 */
3. bind(多播端口，如 SAP 9875)
4. mcast_join(组 224.2.127.254 等)
5. recvfrom 循环，解析 SDP 文本
```

---

## 铁律

接收多播且 **bind 固定端口** → **必须先 SO_REUSEADDR**（Ch 7.5）。

---

## 个人学习总结

（待填）
