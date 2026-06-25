# Aokan City Smart Visual 2.0 Design Spec

Source: `D:\文档\common\前端\规范\城市智慧视觉2.0设计规范`

Use this file as the practical implementation reference for `design-aokan`.

## Layout

- Use a 24-column grid for 1920px desktop pages.
- Grid formula: width 1920px, columns 24, column width 64px, gutter 16px, single-side margin 16px.
- Keep same-function content arranged with consistent front/back layout and visual continuity.
- Separate different functional areas clearly.
- Keep horizontal and vertical spacing consistent across cards, panels, and page sections.
- Base page regions: top bar, left functional menu, content area. Keep content area separated from other regions by 16px by default.
- Expanded menu reference at 1920px: top bar width 1920px, side menu width about 200px, side menu to content gap 20px, content right margin 20px.
- Collapsed menu reference at 1920px: side menu width about 72px, side menu to content gap 20px, content right margin 20px.

## Typography

- Chinese UI font: Microsoft YaHei or Source Han Sans CN.
- English and numeric UI font: Arial.
- Default body text: 14px regular.
- Use regular weight by default. Use bold only for titles or key information.
- Avoid font sizes that are too large or too small for enterprise dashboards.
- Line height equals font size plus line spacing. Example: 24px font with 32px line height means 4px top and bottom leading.

Recommended type scale:

| Use | Size / Weight |
| --- | --- |
| Auxiliary copy | 12px / Regular |
| Body regular | 14px / Regular |
| Body emphasis | 14px / Bold |
| Title regular | 16px / Regular |
| Title regular | 20px / Regular |
| Title regular | 24px / Regular |
| Special title | 32px / Regular |

## Colors

Write CSS colors as hex values.

Text colors:

| Token | Hex | Usage |
| --- | --- | --- |
| Primary | `#1581FF` | Theme color, primary button, active control |
| Text primary | `#2E2F33` | Main body and title text |
| Text secondary | `#49505A` | Secondary title text |
| Text auxiliary | `#AAACB0` | Auxiliary and less important text |
| Text disabled | `#BFBFBF` | Placeholder and disabled text |

Background colors:

| Token | Hex | Usage |
| --- | --- | --- |
| Background primary | `#FFFFFF` | Main container background |
| Background secondary | `#F2F3F5` | Common container fill |
| Background tertiary | `#F4F5F6` | Expanded secondary function background |

Line and fill colors:

| Token | Hex | Usage |
| --- | --- | --- |
| Input border | `#CFD1D2` | Text input and select border |
| Table divider | `#E5E7EB` | Table split line |
| Divider | `#E9E9E9` | Generic divider |
| Dark fill | `#2E2F33` | Linear icons and dark fills |
| Blue fill | `#1581FF` | Linear icons and primary fills |
| Auxiliary icon fill | `#B3B5BA` | Auxiliary prompt icons |
| Table hover fill | `#F3F3F3` | Hover table row fill |

Functional colors:

| Token | Hex | Usage |
| --- | --- | --- |
| Danger | `#F53F3F` | Important warning, error text, danger button or icon |
| Orange | `#E6A23C` | Warning or attention state |
| Yellow | `#F7BA1E` | Warning state |
| Success lime | `#96DB00` | Success or completed prompt |
| Success green | `#67C23A` | Success state |
| Online green | `#00B42A` | Online status |
| Cyan | `#00BEBA` | Auxiliary functional state |
| Link blue | `#3491FA` | Link or secondary action blue |
| Purple | `#722ED1` | Auxiliary functional state |

## Buttons

- Default button height: 32px. Width follows content.
- Text-only button content must be horizontally and vertically centered.
- Provide states: default, hover, active/clicked, disabled.
- Primary button uses blue fill and white text. Secondary button uses border style and blue text for active emphasis.
- Common corner radius: 4px. Compact small button radius: 2px.
- Main button horizontal padding: 16px on both sides.
- Smaller button horizontal padding: 8px on both sides.
- Icon plus text buttons use 32px height, 4px radius, icon-to-text gap about 8px, and tighter inner spacing for compact variants.
- Dropdown button and text button icon gap is about 8px.
- Use loading buttons when click feedback is delayed by data loading. This reduces duplicate clicks and user anxiety.
- Use switch controls for two opposite states such as on/off. Show default, selected, and disabled states.
- Border radius guidance: normal 2px, medium 4px, large 8px, extra large uses pill radius.

## Forms

- Text input and select height: 32px.
- Input width is not fixed; design it around expected content length.
- Common label width reference: about 98px. Field content width reference: about 344px.
- Label-to-control gap reference: 12px.
- Input corner radius: 4px.
- Default border uses input border color. Focus/click state uses primary blue border and focus shadow when supported by the component library.
- Disabled fields use grey background and disabled text color.
- Error state uses danger red border and clear validation text.
- Show placeholder text only when it helps. Hide or clear placeholder when the input is active and contains content, after content is cleared, or when focus behavior would make placeholder noisy.
- Do not show placeholder on first load when it would be misleading, or when an activated input has not yet received focus.
- Validate excessive input length in real time. Do not auto-truncate user text; allow continued input and let the user decide what to remove.
- Set a global maximum input length for normal text inputs to prevent system overload from pasted content.
- Textarea supports character count, such as `0/200`, and should handle long text without breaking surrounding layout.
- Select, multi-select, date, datetime, date range, and time pickers should include default, focused/open, selected, and disabled states.

## Tables

- Use line-separated table presentation for data-dense lists.
- Prefer left alignment for table body content because it matches common reading habits.
- Header content should align consistently with header background spacing.
- Keep default sort order consistent inside the same system.
- Place related information in adjacent columns.
- Avoid overlapping or zero-gap text against table borders.
- Limit displayed characters when needed. Use ellipsis at the end when text exceeds the configured length.
- Support hover row highlight with `#F3F3F3` or the project alias.
- Use pagination for long lists. Default one page shows 10 rows.
- Place table feature/action controls at the upper right or upper area of the table. Place pagination at the lower right.
- Support fixed columns when columns are too many. Fixed columns may be on the left or right.
- Support column visibility settings for dense operational tables.

## Feedback

- Use message/toast for success, warning, info, and error operation feedback. It appears at the top and disappears automatically after about 3 seconds.
- Message reference size: height 48px, corner radius 4px, icon left spacing about 20px, icon-to-text gap about 10px.
- Tooltip is for hover explanations. Reference height 32px and top offset about 12px.
- Notification appears in the lower-right corner for more complex or interactive reminders. Reference width about 320px and height about 88px, with 16px inner spacing.
- Bubble cards are for hover or click floating panels.
- Message modal is a modal dialog used for message prompts, confirmation, and submitting content.
- Dialogs preserve current page state while telling the user what operation to perform. Use them for blocking decisions.
- When a modal is active, lock the current foreground window until it is completed. Do not allow switching to another modal in the same program before completing it.
- Avoid maximize behavior for modal dialogs unless content requires it; leaving too much empty space weakens focus.
- Dialog form reference spacing: header height about 44px, left/right content padding 24px, field vertical gap 24px, label-to-field vertical relation around 8px for validation copy, footer bottom/right padding 24px, footer height about 40px.
