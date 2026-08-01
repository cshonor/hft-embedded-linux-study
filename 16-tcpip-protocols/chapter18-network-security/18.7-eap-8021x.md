# 18.7 网络访问控制与 EAP

> 章级精读：[../study.md#ch18-7](../study.md#ch18-7)

## 本节核心目标

掌握 **802.1X + EAP** 企业有线/Wi-Fi 准入。

---

## 802.1X 角色

| 角色 | 设备 |
|------|------|
| **Supplicant** | 终端 |
| **Authenticator** | 交换机/AP |
| **Authentication Server** | 常 **RADIUS** |

---

## EAP

- **可扩展身份认证**框架（EAP-TLS、PEAP 等）
- 不固定单一算法，由上层选择

---

## 场景

- 企业 Wi-Fi「输账号密码才能上网」的底层机制
