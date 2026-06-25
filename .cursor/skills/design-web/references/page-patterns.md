# Design Web — Page Patterns

Case studies extracted from the source article screenshots. Use for layout structure; do not copy brand assets.

## Pattern Index

| # | Pattern | Example context |
| --- | --- | --- |
| 1 | Feature landing / learning homepage | 听悟-style product home |
| 2 | Admin dashboard with metrics | Customer service / store Q&A config |
| 3 | Service catalog with hero | KUMATE MCP 全周期服务 |
| 4 | Model marketplace | Volcengine 模型广场 |
| 5 | AI assistant entry | Enterprise AI 助手首页 |
| 6 | Tool aggregator | MCP/工具聚合搜索页 |
| 7 | Analytics dashboard | CORE PANEL 通用数据看板 |
| 8 | Finance SaaS dashboard | Spendly 财务收支/税务 |
| 9 | Workbench dashboard | 11款 SaaS 工作台 / 信息降噪 |
| 10 | Map / choropleth component | 世界地图组件对比 |
| 11 | Control big screen | 无人机飞控 / 操控感 |
| 12 | AI tech visual | 科技感 PPT / AI 辅助出图 |

Mode selection: `scenario-router.md`. Article catalog: `article-index.md`.

---

## 1. Feature Landing Homepage

**Use when:** product introduces 3–4 core capabilities on first login.

### Layout

```
[Sidebar icons] | [Top bar: logo · search · utilities · avatar]
                | [Hero: gradient band + headline + optional subtext]
                | [3 large feature cards in a row]
                | [Section: 快捷入口 — grid of smaller link cards]
```

### Feature row cards

- Equal width; glass or white surface; 16px radius.
- Each: colored icon, title (开启实时记录 / 上传音视频 / 搜索链接转写), one-line description.
- Soft 3D or abstract wave decoration in hero background only.

### Quick entry grid

- Smaller horizontal cards: thumbnail left, title + meta right.
- 2–3 columns; compact but spaced (16px gap).

### Colors

- Sidebar: light lavender tint.
- Accents: blue, mint, soft purple per card icon.

---

## 2. Admin Dashboard (Customer Service Config)

**Use when:** configuring many related settings grouped by business domain.

### Layout

```
[Wide sidebar menu] | [Top bar with pill search]
                    | [Tabs: 官方机器人 | 接待设置 | …]
                    | [Summary band: featured card left + metric grid right]
                    | [Section title + 3-column setting cards]
                    | [Repeat sections: 基础店铺问答, 发货物流问题, …]
```

### Summary band

- Left: blue featured card with illustration (robot/status).
- Right: 4–6 metric cells — large number, small gray label (未回复数, 平均响应时长).

### Setting card

- 3-column grid; 40px colored icon square; bold title; one-line gray description.
- Icon color encodes category (orange 热门问题, green 商品问题, blue 物流).

### Behavior

- Tab switch without full page reload.
- Search filters setting cards when list is long.

---

## 3. Service Catalog with Hero (KUMATE-style)

**Use when:** browsing/create/manage a library of services or tools.

### Layout

```
[Top nav: logo · Home · Platform · avatar]
[Sidebar: MCP服务 · 任务管理 · 实例管理 · …]
[Hero gradient: title 全周期 MCP 服务 · search · + 创建MCP服务]
[Filter pills: 服务生成 · 资源管理 · 图像生成 · …]
[3×3 card grid]
```

### Service card anatomy

1. Icon left + green `Tool` badge right.
2. Bold title (1 line).
3. Gray description (2 lines max).
4. Footer: avatar + name + date + status (Created / 已发布).

### Hero

- Horizontal blue-purple gradient; centered title; search below; primary CTA right-aligned in hero row.

---

## 4. Model Marketplace (Volcengine-style)

**Use when:** high-density AI/model catalog with facets.

### Layout

```
[Global top bar: breadcrumb · search · lang · account]
[Deep sidebar: 模型广场 · 推理 · 微调 · 模型仓库 · 评测 · …]
[3 feature banners with 3D/illustration]
[Body split: left facet filters | right model card grid]
[Optional floating utility rail right: 复制ID · 文档 · 对话]
```

### Facet panel (left)

- Groups: 模型类型, 模型功能, 模型架构.
- Each option shows count in gray parentheses.
- Checkbox or click-to-filter.

### Model card

- Gradient logo/icon top-left.
- Model name bold; 2-line description.
- Tags: 大语言模型, etc.
- Update date footer gray.

### Density rule

- Even with many models, keep card internal padding generous; use gray metadata and tags to structure—not shrink whitespace to zero.

---

## 5. AI Assistant Entry

**Use when:** enterprise AI chat is the primary entry point.

### Layout

```
[Sidebar: 新建对话 + recent chat list]
[Center hero: 企业名 AI 助手…]
[Large tabbed prompt box — focal point]
[3 capability cards below: 智能助手 · 办公AI工具集 · 企业知识库]
```

### Tabbed prompt box

- Tabs top: 智能问答, 数据分析, 文件策略.
- Large white input area; model selector (DeepSeek-R1) inside; send icon button.
- Page background: very soft blue-purple gradient.

### Capability cards

- White; soft shadow; icon + title + short description; equal width row.

---

## 6. Tool Aggregator Search Page

**Use when:** discovering integrated tools/MCP services.

### Layout

```
[Centered headline: 聚合优质工具与 MCP 服务…]
[Large centered rounded search]
[Horizontal row of tool category cards: 百度搜索 · 智能搜索生成 · 百度百科 · 智能PPT生成]
```

### Style

- Minimal chrome; maximum center alignment.
- Flat tool cards with clear logos/icons; light blue accent.
- Utility-focused; less sidebar, more search-first.

---

## 7. Analytics Dashboard (CORE PANEL-style)

**Use when:** general admin analytics, team overview, revenue/conversion monitoring.

**Mode:** Finance / data (restrained colors).

### Layout

```
[Sidebar: Dashboard · Admin Tools · Insights · Elements · Pro CTA card]
[Header: search · Hello {name} · lang · bell · profile]
[KPI row: 4 metric cards with sparklines]
[Row 2: Order Revenue line chart (wide) | Team member list (narrow)]
[Row 3: Product Statistics bar | Messages line | Top Conversations bar]
[Row 4: Project overview list with progress bars | Donut status chart]
```

### KPI card

- Title gray 13px; value 24px bold; change `%` green/red with arrow; sparkline right.

### Sidebar Pro CTA

- Dark blue/purple card at bottom; upgrade copy + primary button.

### Style

- Page bg `#F0F2F5`; cards white; primary accent blue-violet `#6366F1`.
- No hero gradient; charts use primary + gray + semantic green/red.

---

## 8. Finance SaaS Dashboard (Spendly-style)

**Use when:** finance, cash flow, tax, balances, transactions — 让复杂数据不再"吓人".

**Mode:** Finance / data (maximum color restraint).

### Layout

```
[Sidebar: Dashboard · Financial Statements · Key Metrics · Tax & Compliance · Account · Support]
[Header: Welcome back, {name} · month picker · share · avatar]
[Slim upgrade banner — light blue, one line + button]
[KPI row: Total Balance · Total Saving · Revenue · Credit]
[Row 2: Cash Flow line chart (Income blue / Expense teal) | Balance gauge semi-circle]
[Row 3: Recent Transactions table | Tax Liabilities summary + segmented bar + list]
```

### KPI card

- Currency value large bold; info icon; `%` trend green up / red down.

### Cash flow chart

- Dual line 6–7 months; tooltip on hover; legend Income/Expense.

### Transactions table

- Search + Filter; columns Order ID, Amount, Status.
- Status chips: green `Paid`, orange `Pending`.

### Tax liabilities widget

- Large total top; horizontal segmented bar (VAT, income tax, services); itemized list below.

### Article copy (verbatim)

- 色彩是外衣，信息是内核
- 好看、好用，但不喧宾夺主
- 红涨绿跌需要被精准传达，但不能满屏幕都是高饱和色块
- 让用户一秒钟就能找到"该赚多少钱"、"哪笔账单超支了"
- 做B端设计最怕自我感动，听劝、务实，把业务场景放到第一位

---

---

## 9. Workbench Dashboard (信息降噪)

**Use when:** 后台工作台、经营看板、Dashboard 首页 — 随心系 11 款 SaaS 灵感。

**Mode:** Workbench（介于 Breathing 与 Finance 之间：可有低饱和背景色，但以 KPI+图表为主）。

### Layout

```
[Sidebar + top bar]
[KPI row: 4 Top Cards — 左上角第一格为最核心指标]
[Row 2: 主趋势图 (area/line 宽) | 次要列表/排行 (窄)]
[Row 3: 2–3 图表卡片 grid — bar / donut / mini table]
[Optional: 待办/动态 feed 窄列]
```

### 四大思路（随心系）

1. **模块化卡片：** 留白 + 低饱和模块底（青绿 `#E8F5F0`、灰蓝 `#EEF2F8`、浅紫 `#F3F0FF`）分隔数据总览/图表/列表。
2. **图形叙事：** 面积图、折线图、圆环图呈现趋势；避免大段裸数字。
3. **核心 KPI 优先：** Top Card 大字号 + 绿涨红跌 % + 可选 sparkline；管理者 1 秒判断健康度。
4. **信息降噪：** 中性背景 + 状态色点缀；注入品牌温度但不喧宾夺主。

### Chart rules

- 每图 1–2 系列色 + 灰网格；tooltip on hover。
- 圆环/ donut 不超过 5 段；legend 置底或右侧。
- 列表模块：紧凑行高，状态用色点或 chip。

### Background tints (module zones)

| Zone | Tint |
| --- | --- |
| Overview | `#EEF2F8` |
| Growth | `#E8F5F0` |
| Alert | `#FFF7ED` |

---

## 10. Map / Choropleth Component

**Use when:** 世界地图、区域分布、Geo 可视化 — 同样是地图质感差的原因。

**Mode:** Map component（可用于 Breathing 或 Workbench 内嵌）。

### Quality factors

| 差 | 好 |
| --- | --- |
| 默认灰底 + 粗边界 | 柔和 choropleth 填充 + 细边界 |
| 所有国家全标注 | 分层：hover/tooltip 显示详情 |
| 高饱和彩虹 | 单色系渐变 3–5 档 |
| 平面无层次 | 轻微阴影或 hover 高亮 |

### Structure

- Legend 固定右下或左下；单位与日期标注清晰。
- 空值区域用 `#F0F0F0`；选中区域加深 1 档。
- B 端内嵌地图高度 320–480px；全屏大屏见 §11。

---

## 11. Control Big Screen (飞控操控感)

**Use when:** 无人机飞控、实时监控、指挥调度大屏 — 要"操控感"。

**Mode:** Control big screen（深色 HUD，非 SaaS 浅色）。

### Layout

```
[Top: 系统状态条 — 连接/信号/电量/告警]
[Left 60–70%: 地图/航迹 — markers 序号 + 轨迹线]
[Right 30–40%: 姿态仪 + 高度/速度/航向 数字仪表]
[Bottom optional: 时间轴 / 事件列表]
[Floating: HUD 告警条 — 失速/越界 red/yellow]
```

### 操控感要素

- **实时反馈：** 数字等宽字体 20–32px；变化时 brief highlight，非整屏闪烁。
- **物理隐喻：** 姿态仪、compass、高度表布局参照真实座舱。
- **低延迟视觉：** 地图 marker 与列表/时间轴联动选中。
- **深色主题：** 背景 `#0B1220`–`#1A2332`；数据青 `#22D3EE`、绿 `#4ADE80`、告警 `#F87171`。

### Don't

- 不用 SaaS 白卡片风格
- 不用复杂 page 渐变
- 装饰 3D 不挡读数

---

## 12. AI Tech Visual (科技感 / PPT)

**Use when:** 发布会 PPT、科技宣传页、AI 辅助视觉 — 手搓 + AI 出图。

**Mode:** AI tech visual（高饱和允许，但需控制焦点）。

### Workflow

1. 参考收集 → 网格构图
2. AI 生成背景/纹理/主视觉（粒子、网格、霓虹边）
3. 手工叠 UI 框、标题、数据（保证可读）
4. 统一色温；全稿最多 1 个发光焦点

### Style

- 深色底 `#0A0E17` + 蓝紫霓虹渐变边
- 细线框 + 低 opacity 网格纹理
- 标题区 glow；正文区保持 WCAG 对比度
- 与 B 端后台区分：宣传可用更炫，操作台仍用 §7–§9

---

## Cross-Pattern Mapping

| Need | Start from pattern |
| --- | --- |
| New SaaS product home | §1 Feature landing |
| Complex admin settings | §2 Admin dashboard |
| Service/tool library | §3 Service catalog |
| AI model listing | §4 Model marketplace |
| Chat-first product | §5 AI assistant entry |
| Integration hub | §6 Tool aggregator |
| General analytics / KPI | §7 Analytics dashboard |
| Finance / billing / tax | §8 Finance SaaS dashboard |
| Workbench / 经营看板 | §9 Workbench dashboard |
| World map / region stats | §10 Map component |
| 飞控 / 监控大屏 / 操控感 | §11 Control big screen |
| 科技感 PPT / AI 视觉 | §12 AI tech visual |

## Article Copy (verbatim highlights)

Use these phrases consistently when writing UX copy for this style:

- 拒绝信息过载
- 呼吸感
- 克制的视觉语言
- 大面积留白搭配清透柔和的渐变色
- 规整的极简卡片网格
- 辨识度极高的彩色"状态标签"
- 降低用户的认知负荷
- 做减法比做加法难得多
- 用最干净利落的样式去传达最高效的功能

## AI Prompt Snippet

```
Follow design-web. Read scenario-router.md first, then page-patterns.md §N.

Workbench §9: 信息降噪, modular cards, KPI top-left, charts over numbers
Finance §7–§8: no gradient, 红涨绿跌 on delta only
Breathing §1–§6: whitespace, soft hero gradient, status tags
Big screen §11: dark HUD, map+instruments, control feel
Map §10: layered choropleth, restrained labels
AI tech §12: dark + single glow focal, AI bg + manual UI
Hybrid: hero §1 + dashboard §9/§8
Principle: 色彩是外衣，信息是内核
```
