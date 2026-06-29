# 部署模块 UI 工具风格改造设计

> 状态：设计中 | 目标版本：Deploy Hub V1

## 1. 目标

将「仪表盘、项目管理、服务器、部署、日志」五个模块从当前 Dashboard 风格统一改造为 Pixso 设计规范中的工具软件风格：

- 去掉 Hero 横幅、大号统计卡片、彩色快捷入口等 Dashboard 元素。
- 采用左侧固定导航 + 顶部 Tab + 右侧白底卡片内容区的布局。
- 统一字体、图标、间距、按钮、卡片、输入框、阴影等视觉细节。
- 一并修复其他页面中 Card 标签（QLabel）显示不全的问题。

参考设计源：Pixso `https://pixso.cn/app/design/i7NgdTXe2h7BW2STrtullg?item-id=2:1`。

## 2. 从 Pixso 提取的设计 Token

### 2.1 色彩

| Token | 色值 | 用途 |
|-------|------|------|
| `PageBg` | `#F0F4F8` | 主工作区背景 |
| `Surface` | `#FFFFFF` | 卡片、侧边栏、顶部栏背景 |
| `Text` | `#1A1A2E` | 标题、重要文字 |
| `TextSecondary` | `#374151` | 正文、用户名 |
| `TextMuted` | `#6B7280` | 导航未选中、次要说明 |
| `Placeholder` | `#9CA3AF` | 占位文字 |
| `Primary` | `#2563EB` | 主按钮、选中态、强调 |
| `PrimaryHover` | `#1D4ED8` | 主按钮悬停 |
| `PrimarySoft` | `#EFF6FF` | 导航选中背景 |
| `Border` | `#E5E9F0` | 边框、分割线 |
| `InputBg` | `#F9FAFB` | 输入区背景 |
| `BotBubble` | `#F3F4F6` | 左侧聊天气泡 |

### 2.2 字体

- 主字体：`Arial` 系列（`Arial-Narrow`、`Arial-Medium`、`Arial-Narrow Bold`）。
- 实际 Qt 环境使用现有系统字体栈：`"Segoe UI", "Microsoft YaHei UI", "Inter", sans-serif`。
- 字号映射：
  - 页面标题：15–16px，Bold
  - 卡片标题/Tab 文字：14px，Medium（选中）/ Narrow（未选中）
  - 正文/输入文字：13–14px
  - 辅助/时间戳：11–12px

### 2.3 间距与尺寸

| 元素 | 尺寸 |
|------|------|
| 左侧导航栏宽度 | 220px |
| 顶部栏高度 | 64px |
| Tab 栏高度 | 56px |
| 导航项高度 | 40px |
| 导航项内边距 | 0 12px |
| 导航项图标 | 16×16px |
| 导航项间距 | 2px |
| 内容区内边距 | 20px |
| 卡片内边距 | 16–20px |
| 卡片间距 | 16px |
| 卡片圆角 | 12px |
| 按钮/输入框圆角 | 8px |
| 小按钮圆角 | 7px |
| 主按钮高度 | 36px |

### 2.4 阴影

- 卡片阴影：`0px 2px 12px 0px rgba(30, 64, 175, 0.094118)`。
- 顶部栏/侧边栏：无阴影，仅使用 1px 边框分隔。

### 2.5 组件样式

#### 2.5.1 侧边栏导航项

- 默认：白底、`#6B7280` 文字、14px、高度 40px、圆角 8px。
- 选中：`#EFF6FF` 背景、`#2563EB` 文字、`Arial-Medium` 字重。
- 图标 16×16，与文字间距 10px。

#### 2.5.2 顶部 Tab

- 容器：白底、高度 56px、下边框 `#E5E9F0`、内边距 0 24px。
- Tab 项：高度 36px、圆角 8px、内边距 0 16px。
- 选中：`#2563EB` 背景、白色文字、14px Medium。
- 未选中：白底、`#6B7280` 文字、14px Narrow。

#### 2.5.3 卡片

- 白底、圆角 12px、内边距 16–20px、卡片阴影。
- 标题 14–16px Bold，正文 13–14px。
- 表格/列表行高适中，信息密度高。

#### 2.5.4 按钮

- 主按钮：高度 36px、圆角 8px、`#2563EB` 背景、白色 14px Medium 文字。
- 次按钮：白底、`#6B7280` 文字、1px `#E5E9F0` 边框（需要时）。
- 工具栏小按钮：28×28 或 36×36、圆角 7–8px。

#### 2.5.5 输入框/文本区

- 背景 `#F9FAFB`、圆角 8px、边框 `#E5E9F0`。
- 聚焦边框 `#2563EB` 或 `#93C5FD`。
- 占位文字 `#9CA3AF`。

## 3. 模块改造要点

### 3.1 仪表盘

- 移除 `dashboardHero`、统计卡网格、快捷操作等 Dashboard 元素。
- 改为「最近部署任务」白底卡片列表，顶部 Tab 切换「全部 / 成功 / 失败」。
- 保留必要状态摘要，但以紧凑表格形式呈现，而非大卡片。

### 3.2 项目管理

- 左侧为项目列表（类似导航项样式），右侧为项目详情卡片。
- 新增/编辑按钮使用 Pixso 主按钮样式。
- 表单项使用统一输入框样式。

### 3.3 服务器

- 服务器列表使用卡片式表格，状态标签圆角小 pill。
- 连接测试按钮使用主按钮样式。
- 服务器表单放入白底卡片。

### 3.4 部署

- 部署页面改为「选择项目 + 选择服务器 + 部署参数」卡片表单。
- 部署进度/日志以底部或右侧卡片展示，避免弹窗浮层。
- 操作按钮统一使用主按钮。

### 3.5 日志

- 日志列表与日志内容分栏：左侧列表、右侧详情卡片。
- AI 分析面板与聊天内容参考 Pixso 消息气泡样式（左侧灰泡、右侧蓝渐变泡、头像圆角 10px）。

## 4. Card Label 显示不全修复

- 统一为 Card 标题/标签 QLabel 设置 `setWordWrap(false)`，并配置 `Qt::ElideRight` 或 `QFontMetrics` 截断提示。
- 在布局中为标签分配合理 `stretch` 或 `minimumSize`，避免被图标/按钮挤压。
- 对需要完整文案的场景使用 `setToolTip()`。

## 5. 动画与交互

- 页面切换、Tab 切换：不使用夸张动画，保持工具软件的即时反馈。
- 按钮悬停：背景色变化 100–150ms。
- 卡片/输入框聚焦：边框色变化，不使用阴影扩散动画。

## 6. 待实现文件范围

- `src/ui/Theme.h`：补充/调整 Token。
- `src/ui/style.qss`：新增/覆盖全局组件样式。
- `src/ui/AppStyle.cpp`：必要时同步占位符映射。
- 模块 Widget：`MainWindow.cpp/h`、`ProjectManagerWidget`、`ServerManagerWidget`、`DeployWorker` 相关 UI、`DeploymentLogDialog`、`LogAiAnalysisWidget`、`CommonToolsWidget` 等。
- QML 壳层：`src/qml/DeployHub/MainShell.qml`、`DhSidebar.qml`、`DhModuleTabBar.qml` 等。

## 7. 验证

- `scripts/build-release.ps1` 构建通过。
- `ctest --test-dir build-release --output-on-failure` 测试通过。
- 重点检查 Card 标签不被截断、Tab 和导航选中态正确。
