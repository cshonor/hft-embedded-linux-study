## 2. 经纪公司内部架构 (Structure of a Brokerage Firm)

大型经纪公司通常分三块：

### 2.1 前台 (Front Office)

直接面向客户：

| 部门 | 职能 |
|------|------|
| **销售与交易 (Sales & Trading)** | 拉订单、执行、客户关系 |
| **企业融资 (Investment Banking)** | 协助 **发行证券** |
| **研究 (Research)** | 投资分析与报告 |

### 2.2 后台 (Back Office)

支撑交易运转：

| 部门 | 职能 |
|------|------|
| 账户维护 | 持仓、成本、报表 |
| **清算与结算 (Clearing & Settlement)** | 与 DTCC/托管行对接 |
| 信息系统 | OMS、风控、报表 |
| **信用管理 (Credit management)** | 保证金、授信 |
| **合规 (Compliance)** | 监管报送、best execution 审计 |

### 2.3 自营 (Proprietary Operations)

| 业务 | 说明 |
|------|------|
| **现金管理** | 客户闲置资金的投资 |
| **融券 (Securities lending)** | 借出证券给 **做空者** |
| **风险管理** | 公司级风险敞口 |

| HFT 视角 |
|----------|
| HFT  firm's **tech stack** 多在前台 execution；**back office latency** 影响 end-of-day margin、locate |
| **Prime brokerage** 把 credit、securities lending、cross-margin 打包——机构 HFT 常见模式 |
| 合规与 **CAT / MiFID II best ex reporting** 是 post-trade 的硬约束 |

---
