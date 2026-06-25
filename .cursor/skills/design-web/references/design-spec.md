# Design Web — B-End SaaS Breathing-Room Spec

Sources:

| Article | URL |
| --- | --- |
| 呼吸感 B端 SaaS | https://mp.weixin.qq.com/s/AsAwJdFrBv8xjzbZdOS_jg |
| 财务 SaaS 复杂数据 | https://mp.weixin.qq.com/s/NT2yjDOTMNayZECofqc_4A |

Design works are for learning reference only; copyrights belong to original creators.

Mode selection: read `scenario-router.md` first. Full catalog: `article-index.md`.

## Design Philosophy

| Principle | Meaning |
| --- | --- |
| 呼吸感 (breathing room) | Generous whitespace and calm margins so complex business UI feels relaxed |
| 克制 (restraint) | Limit colors, decoration, and visual noise; avoid "闷" and "挤" |
| 做减法 | Remove before adding; hierarchy through space and type, not element stacking |
| 高级感 (premium feel) | Soft gradients + clean cards + clear tags, not heavy borders or dense gray blocks |
| 降低认知负荷 | Colored status tags and section grouping help users scan large catalogs quickly |
| 色彩是外衣，信息是内核 | Color is decoration; data logic and accuracy are the core |
| 不喧宾夺主 | 好看、好用，但不抢数据风头 |
| 听劝务实 | Backend/data users reject "美颜滤镜"; put business scenario first |

## Visual Direction

- Apply consumer-grade polish to professional B-end tools.
- Light mode default: white cards on `#F5F7FA` or `#F8F9FB` workspace.
- Soft pastel accents: lavender purple, cloud blue, mint green — never full-saturation fills on large areas.
- Optional glassmorphism on hero/feature cards: frosted surface, light border, subtle shadow.
- Thin-line icons; one icon style per page area.
- Rounded corners: cards 12–20px; buttons/inputs 8–12px; search pill fully rounded.

### Breathing SaaS vs Finance data

| Aspect | Breathing SaaS | Finance / data dashboard |
| --- | --- | --- |
| Page gradient | Soft hero band allowed | None; flat `#F5F7FA` workspace |
| Glassmorphism | Optional on feature cards | Avoid |
| Accent use | Lavender, blue, mint on icons/hero | Single primary blue + semantic red/green |
| Primary widgets | Feature cards, catalog grid | KPI cards, line/bar/gauge charts, tables |
| Color budget | Higher on marketing zones | Minimal; 红涨绿跌 on delta/status only |

### Workbench (§9) module tints

| Token | Hex | Usage |
| --- | --- | --- |
| Zone overview | `#EEF2F8` | Data summary cards |
| Zone growth | `#E8F5F0` | Trend metrics |
| Zone alert | `#FFF7ED` | Attention areas |
| Chart primary | `#6366F1` | Main series |
| Chart secondary | `#14B8A6` | Comparison series |

### Control big screen (§11) tokens

| Token | Hex | Usage |
| --- | --- | --- |
| Screen bg | `#0B1220` | Full page |
| Panel bg | `#1A2332` | Instruments |
| Data cyan | `#22D3EE` | Live values |
| Data green | `#4ADE80` | Normal |
| Alert red | `#F87171` | Critical |
| Alert yellow | `#FBBF24` | Warning |

Use monospace/tabular numbers on instrument panels.

## Layout

### Page shell

- **Left sidebar:** narrow icon rail (48–64px) or wider menu (200–240px) with hierarchical items.
- **Top bar:** logo/brand left, centered or right search, utility icons (download, help, notification, settings), user avatar right.
- **Main content:** padded 20–32px; max readable width optional for marketing-style heroes.
- **Spacing scale:** 8, 12, 16, 20, 24, 32, 48px.

### Region patterns

| Region | Height / width | Notes |
| --- | --- | --- |
| Top bar | 48–56px | White or translucent; bottom border or soft shadow |
| Sidebar | 200–240px expanded | Light lavender `#E8EAF6` or neutral `#F5F7FA` |
| Hero band | 160–240px | Soft blue-purple gradient; headline + search/CTA |
| Card grid gap | 16–24px | 3-column default for catalogs; 2–4 for feature rows |
| Section gap | 32–48px | Between hero, filters, and grid |

### One-screen preference

- Keep shell (sidebar + top bar) fixed when content is long.
- Scroll inside card grids, facet lists, or table bodies.
- Avoid page-level scroll for routine dashboard landing views at 1920×1080 when content fits.

## Typography

- Font stack: `"PingFang SC", "Microsoft YaHei", "Source Han Sans SC", Arial, sans-serif`.
- Default body: 14px regular, `#1F1F1F` or `#2E2F33`.
- Secondary/metadata: 12–13px, `#8C8C8C` or `#666666`.
- Section title: 16–18px medium.
- Page/hero title: 24–32px medium or semibold; do not use oversized bold for every heading.
- Card title: 14–16px semibold; description 12–14px regular gray.
- Line height: body ~1.5; compact card text ~1.4.

## Color Tokens

| Token | Hex (approx) | Usage |
| --- | --- | --- |
| Primary blue | `#4F6EF7` / `#1677FF` | Primary button, links, active nav |
| Soft purple | `#E8EAF6` / `#EDE9FE` | Sidebar tint, hero gradient start |
| Soft blue | `#E0E7FF` / `#DBEAFE` | Hero gradient, feature accents |
| Mint green | `#D1FAE5` / `#6EE7B7` | Success tags, positive badges |
| Page background | `#F5F7FA` / `#F8F9FB` | Workspace |
| Card surface | `#FFFFFF` | Cards, dialogs, top bar |
| Text primary | `#1F1F1F` | Titles, main content |
| Text secondary | `#666666` | Descriptions, metadata |
| Text placeholder | `#BFBFBF` | Input placeholder |
| Border subtle | `#E8E8E8` / `#E5E7EB` | Card border, dividers |
| Shadow soft | `0 4px 24px rgba(0,0,0,0.06)` | Card elevation |

### Hero gradient example

```css
.hero-band {
  background: linear-gradient(135deg, #e0e7ff 0%, #ede9fe 50%, #f5f3ff 100%);
}
```

### Glass card example

```css
.feature-card {
  background: rgba(255, 255, 255, 0.72);
  backdrop-filter: blur(12px);
  border: 1px solid rgba(255, 255, 255, 0.8);
  border-radius: 16px;
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.06);
}
```

## Status Tags

Use filled or soft-fill chips with high recognition; keep height 22–28px, radius 4–6px.

| Status | Background | Text | Example label |
| --- | --- | --- | --- |
| Published | `#D1FAE5` | `#059669` | 已发布 |
| Reviewing | `#FEF3C7` | `#D97706` | 审核中 |
| Draft | `#F3F4F6` | `#6B7280` | 草稿 |
| Tool/类型 | `#DCFCE7` | `#16A34A` | Tool |
| Model type | `#EDE9FE` | `#7C3AED` | 大语言模型 |

Place tags top-right of cards or inline after title; limit 2–3 tags per card.

## Finance And Data Colors

Use semantic color sparingly on financial dashboards.

| Semantic | Color | Usage |
| --- | --- | --- |
| Positive / up / paid | `#22C55E` / `#059669` | 涨, +%, Paid, 已支付 |
| Negative / down / overdue | `#EF4444` / `#DC2626` | 跌, -%, Overdue, 超支 |
| Pending / warning | `#F59E0B` | Pending, 待处理, 审核中 |
| Primary data series | `#4F6EF7` | Income line, primary bar |
| Secondary series | `#14B8A6` / `#06B6D4` | Expense line, comparison |
| Neutral text/metadata | `#666666` | Axis labels, table secondary |

Rules:

- Do not fill entire cards with red/green backgrounds.
- KPI trend: small colored percentage + optional sparkline; card surface stays white.
- Chart palette: 1–2 series colors + gray grid; avoid rainbow.
- Status column in tables: soft-fill chip (green Paid, orange Pending).

## Components

### Primary button

- Height 36–40px; blue fill; white text; radius 8px.
- Hero CTA may be larger (40–44px) with icon.

### Search input

- Pill shape (full radius); height 36–40px; light border; search icon left.
- Hero search may be wider (480–640px) centered.

### Feature card (large)

- Icon 32–40px colored square or rounded container top-left.
- Title 16px semibold; description 13–14px gray, max 2 lines ellipsis.
- Optional arrow or hover lift (`translateY(-2px)` + stronger shadow).

### Catalog card (medium)

- Logo/icon top-left; status badge top-right.
- Title + 2-line description.
- Footer: avatar + author/name + date + status text.
- Grid: 3 columns desktop; 2 tablet; 1 mobile.

### Metric card

- Large number 24–32px; label 12–13px gray above or below.
- White surface; minimal border; no heavy chart chrome unless needed.

### Setting/action card (admin)

- 3-column grid; colorful 40px square icon; title + one-line description.
- Icon colors differentiate categories (orange hot issues, green product, blue logistics).

### Tabbed prompt box (AI entry)

- Large white container; tabs on top (智能问答, 数据分析, 文件策略).
- Model selector dropdown inside; send button (icon) bottom-right.
- Soft page gradient behind; feature cards in row below.

### KPI card (finance)

- White card; subtle border; padding 16–20px.
- Label 13px gray top; value 24–28px bold; trend `%` with color + mini sparkline right.
- Four cards equal width in top row; gap 16px.

### Chart widget

- White card container; title 16px semibold + optional period selector.
- Line chart: smooth curve, light gray grid, tooltip on hover.
- Bar chart: grouped bars, max 3–4 colors.
- Gauge / semi-circle: center percentage, legend below.

### Transaction table

- Columns: Order ID, Amount, Status (chip), Date optional.
- Toolbar: search + Filter button above table.
- Row height compact ~48px; status chip right-aligned.

## Navigation

- Sidebar: icon + label; active item blue text/icon + light blue/lavender bg pill.
- Top tabs for sub-sections within a module (e.g. 官方机器人, 接待设置).
- Breadcrumbs on deep admin pages when hierarchy exceeds 2 levels.
- Filter pills horizontal below hero: text categories, active underline or filled pill.

## Interaction

- Card hover: subtle shadow increase + optional 2px lift; cursor pointer when clickable.
- Buttons: default, hover darken 8%, disabled 40% opacity.
- Submit actions: loading state on primary button; debounce duplicate clicks.
- Empty catalog: centered illustration + `暂无数据` + optional CTA.
- Destructive actions: confirm dialog; danger color on confirm only.

## Anti-Patterns

| Avoid | Prefer |
| --- | --- |
| Filling every pixel with charts and tables | Hero + grid; scroll inner areas |
| Heavy dark sidebar + dense tables (legacy admin) | Light sidebar + white cards |
| Rainbow tag colors without meaning | Semantic status tag palette |
| Multiple gradient backgrounds per viewport | One hero gradient band |
| 4px radius everywhere mixed with 20px | Consistent radius tier per component type |
| Outline-only icons on white with no hierarchy | Icon + title + tag structure |
| Gradient finance dashboard | Flat bg + white chart cards |
| Full-screen saturated 红涨绿跌 | Semantic color on delta/status only |
| Self-indulgent decoration | 1-second path to core metric |

## CSS Token Sketch

```css
:root {
  --web-primary: #4f6ef7;
  --web-primary-soft: #e0e7ff;
  --web-purple-soft: #ede9fe;
  --web-bg-page: #f5f7fa;
  --web-bg-card: #ffffff;
  --web-sidebar: #e8eaf6;
  --web-text-primary: #1f1f1f;
  --web-text-secondary: #666666;
  --web-border: #e8e8e8;
  --web-radius-card: 16px;
  --web-radius-control: 8px;
  --web-shadow-card: 0 4px 24px rgba(0, 0, 0, 0.06);
  --web-font-family: "PingFang SC", "Microsoft YaHei", Arial, sans-serif;
}
```

## Review Checklist

- First impression: relaxed, not crowded.
- Whitespace ratio higher than typical legacy B-end.
- Gradients limited and soft.
- Status tags scannable; hierarchy obvious in 3 seconds.
- Sidebar/top bar/content aligned on spacing grid.
- Cards share radius, shadow, and padding conventions.
- No copyrighted logos or artwork copied into deliverables.
