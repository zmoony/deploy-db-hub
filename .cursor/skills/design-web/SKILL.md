---
name: design-web
description: Applies 随心系 B-end web design across five auto-selected modes — breathing SaaS homepages, workbench dashboards (信息降噪), finance KPI panels, control big screens (飞控操控感), map components, and AI tech visuals. Use when building or reviewing SaaS UI, admin dashboards, financial reports, data visualization, world maps, drone/control HUD screens, tech PPT layouts, or when the user mentions 随心系, 呼吸感, 工作台, 信息降噪, 财务后台, 红涨绿跌, 操控感, 大屏, 世界地图, 科技感, too fancy, or complex data that should not feel scary.
---

# Design Web

## Purpose

Implement or review frontend UI against the **随心系** B-end design article series. Five complementary modes share one principle:

**色彩是外衣，信息是内核**

## Workflow

1. **Route scenario:** read `references/scenario-router.md` → pick mode (Breathing / Workbench / Finance / Big screen / AI tech / Map).
2. **Find source article:** `references/article-index.md` (随心系 full catalog + key takeaways).
3. **Layout anatomy:** `references/page-patterns.md` §1–§12.
4. **Tokens:** `references/design-spec.md`.
5. Verify 1-second scan to core metric/action.

## Scenario Quick Map

| Scenario | Pattern | Mode |
| --- | --- | --- |
| SaaS / AI 产品首页 | §1–§6 | Breathing |
| 工作台 / Dashboard | §9 | Workbench |
| 财务 / KPI / 报表 | §7–§8 | Finance |
| 世界地图 / 区域分布 | §10 | Map |
| 飞控 / 监控大屏 | §11 | Big screen |
| 科技感 PPT / AI 视觉 | §12 | AI tech |
| 模型/服务目录 | §3–§4 | Breathing |
| 混合产品 | Hero §1 + Tab §9/§8 | Hybrid |

## Core Rules

- **信息降噪** — remove before decorating; 做减法比做加法难.
- **Breathing:** soft hero gradients, glass cards, status tags (已发布/审核中).
- **Workbench:** modular cards, low-sat tints, KPI top-left, charts over raw numbers.
- **Finance:** no page gradients; 红涨绿跌 on delta/status only; 好看、好用，但不喧宾夺主.
- **Big screen:** dark HUD, map + instruments, 操控感 via real-time readable numbers.
- **Map:** choropleth layers, restrained labels, tooltip detail on demand.
- **AI tech:** one glow focal max; AI background + manual UI overlay.
- Backend users reject "美颜滤镜" — listen when feedback says too fancy.
- Designs are learning references only; do not ship copyrighted artwork.

## Validation Checklist

- Correct mode for user scenario (not gradient finance or light SaaS on fly-control HUD).
- Core answer in ~1s (revenue, overspend, alert status, primary action).
- Workbench: KPI row + chart grid, not number wall.
- Finance: semantic red/green on deltas only.
- Big screen: dark theme, legible monospace numbers, map-list sync.
- Whitespace/cards consistent; lint/preview when applicable.

## Sources

随心系微信公众号 · 详见 `references/article-index.md`

| Key article | URL |
| --- | --- |
| 呼吸感 SaaS | https://mp.weixin.qq.com/s/AsAwJdFrBv8xjzbZdOS_jg |
| 财务 SaaS | https://mp.weixin.qq.com/s/NT2yjDOTMNayZECofqc_4A |

## References

- `references/scenario-router.md` — **start here** (智能场景路由)
- `references/article-index.md` — 随心系文章目录与要点
- `references/page-patterns.md` — §1–§12 布局案例
- `references/design-spec.md` — tokens, color, components
