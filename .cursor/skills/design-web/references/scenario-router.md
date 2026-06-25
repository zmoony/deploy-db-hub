# Design Web — Scenario Router

**Start here.** Read `article-index.md` for full 随心系 article catalog.

## Quick Decision

| User says / context | Mode | Read |
| --- | --- | --- |
| 工作台、Dashboard、经营看板、11款SaaS、信息降噪 | **Workbench** | `page-patterns.md` §9 |
| 财务、报表、KPI、资金、账单、税务、收支 | **Finance data** | §7–§8, `design-spec.md` Finance |
| 后端 too fancy、色彩太多、数据准确优先 | **Restrained data** | Finance data mode |
| AI 平台、模型广场、MCP、助手、工具聚合 | **Breathing SaaS** | §1–§6 |
| 呼吸感、渐变、玻璃拟态、SaaS 首页 | **Breathing SaaS** | §1, `design-spec.md` Visual |
| 世界地图、区域分布、地图组件 | **Map component** | §10 |
| 大屏、飞控、无人机、监控、操控感、HUD | **Control big screen** | §11 |
| 科技感、PPT、AI 出图、发布会、霓虹 | **AI tech visual** | §12 |
| 客服配置、设置中心 | **Admin config** | §2 |
| 服务/模型目录、状态标签 | **Catalog grid** | §3–§4 |
| 不确定 | **Default** | KPI/charts → Workbench or Finance; else Breathing |

## Five Modes

### A. Breathing SaaS

**Source:** 随心系 · 呼吸感 B端 SaaS  
**Signals:** 呼吸感, 渐变, 玻璃拟态, 模型, MCP, 助手, 快捷入口  
**Do:** hero 柔和渐变, 功能卡片, 彩色状态标签  
**Don't:** 数据区堆满图表

### B. Workbench / 信息降噪

**Source:** 随心系 · 11款高颜值 SaaS 工作台  
**Signals:** 工作台, Dashboard, 数据总览, 经营看板, 模块化卡片  
**Do:** 卡片模块化, 低饱和背景(青绿/灰蓝/浅紫), KPI 左上角, 图表讲趋势, 绿涨红跌  
**Don't:** 纯数字墙, 彩虹图表

### C. Finance / Data Dashboard

**Source:** 随心系 · 财务 SaaS  
**Signals:** 财务, 红涨绿跌, 账单, 收支, 经营趋势, 资金结构  
**Do:** 无页面渐变, 白卡片, KPI+图表+表格, 语义色仅用于 delta/状态  
**Don't:** 满屏高饱和, 复杂渐变

### D. Control Big Screen

**Source:** 随心系 · 无人机飞控可视化  
**Signals:** 大屏, 飞控, 操控感, 地图航迹, 姿态仪, HUD, 实时监控  
**Do:** 深色底, 高等宽数字, 地图+仪表分栏, 告警红/黄, 低延迟视觉反馈  
**Don't:** 装饰 3D, 浅色 SaaS 风格

### E. AI Tech Visual

**Source:** 随心系 · 红飞 AI 科技设计 / PPT  
**Signals:** 科技感, PPT, AI 辅助出图, 发布会, 霓虹, 手搓  
**Do:** 深色+单焦点发光, 网格/粒子低 opacity, 手工 UI 框叠 AI 背景  
**Don't:** 全屏发光, 牺牲可读性

## Core Principles (all modes)

> 色彩是外衣，信息是内核

- 信息降噪; 做减法比做加法难  
- 1 秒找到核心答案  
- 好看、好用，但不喧宾夺主  
- 听劝、务实，业务场景第一位

## Hybrid Pages

| Zone | Style |
| --- | --- |
| 产品落地页 | Breathing §1 |
| 登录后 Dashboard | Workbench §9 or Finance §7–§8 |
| 数据分析 Tab | Finance data |
| 监控/飞控页 | Control big screen §11 |
| 发布/PPT 宣传 | AI tech visual §12 |

## Invocation Flow

```
1. Read scenario-router (this file)
2. Match user keywords → mode A–E
3. Open page-patterns.md §N
4. If need article context → article-index.md
5. Tokens → design-spec.md
```

## AI Prompt Snippet

```
Use design-web. Read scenario-router.md → pick mode → page-patterns §N.

Workbench §9: 信息降噪, modular cards, KPI top-left, charts over numbers
Finance §7–§8: no gradient, 红涨绿跌 on delta only
Breathing §1–§6: whitespace, soft gradient hero, status tags
Big screen §11: dark HUD, map+instruments, control feel
Map §10: layered choropleth, restrained labels
AI tech §12: dark + single glow focal, AI bg + manual UI
Principle: 色彩是外衣，信息是内核
```
