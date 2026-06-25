---
name: design-aokan
description: Apply the Aokan city smart visual 2.0 frontend design system. Use when building, reviewing, or refactoring web UI for city smart, video cloud, monitoring, operations, admin dashboards, data tables, forms, buttons, feedback messages, dialogs, or any frontend page that must follow the local "城市智慧视觉2.0设计规范".
---

# Design Aokan

## Purpose

Use this skill to implement or review frontend UI against the Aokan city smart visual 2.0 design system.
Prioritize consistency, information density, readable Chinese UI, and conservative enterprise dashboard patterns.

## Workflow

1. Identify the page type: dashboard layout, list/table page, form page, dialog, or mixed workflow.
2. Read `references/design-spec.md` before making visual or component decisions.
3. Reuse the project's existing components and design tokens first. Only add new tokens when the project has no equivalent.
4. Normalize layout, typography, color, spacing, and component states before tuning details.
5. Verify the result against the checklist below and the affected framework's normal checks.

## Core Rules

- Use the 24-column grid on desktop pages, with 16px gutters and 16px single-side margin by default.
- Keep the base Chinese font at 14px. Use Microsoft YaHei or Source Han Sans CN for Chinese text and Arial for numbers and non-special English text.
- Use hex color values in CSS. Do not use warning red/yellow/orange as normal body text colors.
- Keep common controls 32px high unless the existing component library has an equivalent established size.
- Keep button labels, access keys, and dialog placement consistent when the same action appears in multiple windows.
- Prefer clear left alignment for table body content and stable column order for related data.
- Avoid unrelated redesign. Match existing project style unless it conflicts with the design system.

## Implementation Notes

- For Ant Design, Element Plus, or similar component libraries, map these rules to theme tokens, CSS variables, or wrapper classes instead of hardcoding repeated inline styles.
- For layout-heavy pages, define page-level spacing first: top bar, left menu, content area, card/table area, and footer/pagination.
- For forms, keep labels predictable, inputs 32px high, placeholders contextual, and validation immediate but not disruptive.
- For feedback, choose the lightest component that communicates the result: message/toast for transient feedback, notification for richer background reminders, modal/dialog for blocking confirmation.

## Validation Checklist

- Layout uses consistent page regions, spacing, and grid rhythm.
- Text hierarchy uses the documented font sizes and weights.
- Colors use documented semantic tokens or project aliases that map to them.
- Buttons, inputs, selects, tables, pagination, tooltips, notifications, and dialogs include required states.
- Table text truncates with ellipsis where needed and does not break row height unexpectedly.
- Destructive, blocking, or system-state-changing actions use explicit confirmation.
- Normal project verification has run, such as lint, typecheck, unit tests, storybook, or visual preview.

## Reference

Read `references/design-spec.md` for extracted values and component guidance from the local source images:
`D:\文档\common\前端\规范\城市智慧视觉2.0设计规范`.
