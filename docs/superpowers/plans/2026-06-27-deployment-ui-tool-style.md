# 部署模块 UI 工具风格改造实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 把仪表盘、项目管理、服务器、部署、日志五个模块按 Pixso 设计规范统一改造成工具软件风格，并修复 Card QLabel 显示不全问题。

**Architecture:** 先更新全局 Theme Token 与 style.qss，统一颜色/圆角/阴影/按钮/卡片；再改造 QML 主壳（侧边栏、顶部 Tab、内容区）；最后逐个迁移 Widget 业务页，移除 Dashboard 元素，复用统一卡片和按钮样式。

**Tech Stack:** C++17 / Qt 6 / QML / QSS / CMake

---

## 文件结构

| 文件 | 职责 |
|------|------|
| `src/ui/Theme.h` | 颜色/圆角/间距 Token |
| `src/ui/style.qss` | 全局 QSS 与 @C1@~@C20@ 占位符样式 |
| `src/ui/AppStyle.cpp` | 占位符替换映射 |
| `src/qml/DeployHub/MainShell.qml` | 顶层布局（侧边栏 + 内容区） |
| `src/qml/DeployHub/DhSidebar.qml` | 左侧模块导航 |
| `src/qml/DeployHub/DhModuleTabBar.qml` | 顶部模块 Tab 栏 |
| `src/qml/DeployHub/DhLineTabBar.qml` | 子页面 Tab 栏 |
| `src/ui/MainWindow.cpp/h` | 主窗口、模块切换、仪表盘内容 |
| `src/ui/ProjectManagerWidget.cpp/h` | 项目管理页 |
| `src/ui/ServerManagerWidget.cpp/h` | 服务器管理页 |
| `src/ui/DeployWorker.cpp/h` | 部署任务页/进度展示 |
| `src/ui/DeploymentLogDialog.cpp/h` | 部署日志弹窗 |
| `src/ui/LogAiAnalysisWidget.cpp/h` | AI 日志分析/聊天 |

---

## Task 1: 更新全局 Theme Token

**Files:**
- Modify: `src/ui/Theme.h`

**目标：** 让 Token 与 Pixso 设计对齐，避免各页面硬编码颜色。

- [ ] **Step 1: 修改 `ThemeColors`**

```cpp
namespace ThemeColors {
constexpr auto PageBg = "#F0F4F8";
constexpr auto Surface = "#FFFFFF";
constexpr auto Text = "#1A1A2E";
constexpr auto TextSecondary = "#374151";
constexpr auto TextMuted = "#6B7280";
constexpr auto Placeholder = "#9CA3AF";
constexpr auto Primary = "#2563EB";
constexpr auto PrimaryHover = "#1D4ED8";
constexpr auto PrimarySoft = "#EFF6FF";
constexpr auto PrimarySoftHover = "#DBEAFE";
constexpr auto Border = "#E5E9F0";
constexpr auto InputBorder = "#E5E9F0";
constexpr auto InputHover = "#93C5FD";
constexpr auto InputBg = "#F9FAFB";
constexpr auto Success = "#16A34A";
constexpr auto Warning = "#F59E0B";
constexpr auto Danger = "#EF4444";
constexpr auto DangerHover = "#DC2626";
constexpr auto SidebarBg = "#FFFFFF";
constexpr auto SidebarText = "#6B7280";
constexpr auto SidebarTextSelected = "#2563EB";
}
```

- [ ] **Step 2: 扩展 `ThemeRadius` 增加卡片大圆角**

```cpp
namespace ThemeRadius {
constexpr int Small = 6;
constexpr int Medium = 8;
constexpr int Large = 10;
constexpr int Card = 12;
constexpr int Pill = 999;
}
```

- [ ] **Step 3: 提交**

```bash
git add src/ui/Theme.h
git commit -m "chore(ui): align theme tokens with Pixso tool-style design"
```

---

## Task 2: 扩展 style.qss 占位符与全局组件样式

**Files:**
- Modify: `src/ui/style.qss`
- Modify: `src/ui/AppStyle.cpp`（如需新占位符）

**目标：** 让卡片、按钮、Tab、输入框、侧边栏复用统一样式。

- [ ] **Step 1: 在 `AppStyle.cpp` 中为卡片圆角/输入背景/卡片阴影增加占位符**

在 `replacements[]` 数组末尾追加（保留原有 1-20 不变，新增 21-23）：

```cpp
{QStringLiteral("@C21@"), QString::number(ThemeRadius::Card)},
{QStringLiteral("@C22@"), QString::fromLatin1(kInputBg)},
{QStringLiteral("@C23@"), QStringLiteral("0px 2px 12px 0px rgba(30, 64, 175, 0.094118)")},
```

- [ ] **Step 2: 在 `style.qss` 新增/覆盖组件样式**

```qss
/* 卡片 */
QFrame#toolCard,
QFrame#contentPanel,
QFrame#dashboardStatCard,
QFrame#dashboardQuickAction,
QFrame#serviceInstanceBanner,
QFrame#dashboardHero {
    background-color: #FFFFFF;
    border: 1px solid @C4@;
    border-radius: @C21@px;
}

/* 顶部 Tab 栏 */
QWidget#moduleTabBar,
QWidget#lineTabBar {
    background-color: #FFFFFF;
    border: none;
    border-bottom: 1px solid @C4@;
    border-radius: 0;
    padding: 0 24px;
    min-height: 56px;
    max-height: 56px;
}

/* Tab 按钮 */
QPushButton#moduleTabButton {
    background-color: #FFFFFF;
    color: @C3@;
    border: none;
    border-radius: @C5@px;
    padding: 0 16px;
    min-height: 36px;
    font-size: 14px;
}
QPushButton#moduleTabButton:checked,
QPushButton#moduleTabButton:hover {
    background-color: @C6@;
    color: #FFFFFF;
}

/* 主按钮 */
QPushButton#primaryButton,
QPushButton#toolPrimaryButton {
    background-color: @C6@;
    color: #FFFFFF;
    border: none;
    border-radius: @C5@px;
    padding: 0 16px;
    min-height: 36px;
    font-size: 14px;
    font-weight: 500;
}
QPushButton#primaryButton:hover,
QPushButton#toolPrimaryButton:hover {
    background-color: @C10@;
}

/* 次按钮 */
QPushButton#secondaryButton {
    background-color: #FFFFFF;
    color: @C3@;
    border: 1px solid @C4@;
    border-radius: @C5@px;
    padding: 0 16px;
    min-height: 36px;
    font-size: 14px;
}

/* 侧边栏导航 */
QListWidget#sidebarNav::item {
    min-height: 40px;
    border-radius: @C5@px;
    padding: 0 12px;
    margin: 1px 0;
    color: @C17@;
    font-size: 14px;
}
QListWidget#sidebarNav::item:selected {
    background-color: @C19@;
    color: @C20@;
    font-weight: 500;
}
QListWidget#sidebarNav::item:hover {
    background-color: @C18@;
}

/* 输入框 */
QLineEdit,
QPlainTextEdit,
QTextEdit {
    background-color: @C22@;
    border: 1px solid @C4@;
    border-radius: @C5@px;
    padding: 8px 12px;
    color: @C2@;
    font-size: 13px;
}
QLineEdit:focus,
QPlainTextEdit:focus,
QTextEdit:focus {
    border: 1px solid @C6@;
}
```

- [ ] **Step 3: 提交**

```bash
git add src/ui/style.qss src/ui/AppStyle.cpp
git commit -m "feat(ui): add Pixso-style cards, tabs, buttons and inputs to global qss"
```

---

## Task 3: 改造 QML 主壳（侧边栏 + 顶部 Tab）

**Files:**
- Modify: `src/qml/DeployHub/MainShell.qml`
- Modify: `src/qml/DeployHub/DhSidebar.qml`
- Modify: `src/qml/DeployHub/DhModuleTabBar.qml`

**目标：** 让 QML 壳层匹配 Pixso 布局：左侧 220px 白底导航、顶部 56px Tab 栏、内容区 #F0F4F8 背景。

- [ ] **Step 1: 修改 `MainShell.qml` 背景色与尺寸**

将根容器背景改为 `#F0F4F8`，侧边栏宽度固定 `220`，顶部栏高度固定 `56`。

```qml
Rectangle {
    id: root
    color: "#F0F4F8"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        DhSidebar {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            DhModuleTabBar {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
            }

            StackLayout {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
```

- [ ] **Step 2: 修改 `DhSidebar.qml` 导航项样式**

- 列表项高度 40，圆角 8。
- 未选中：文字 `#6B7280` 14px；选中：背景 `#EFF6FF`，文字 `#2563EB`，font.weight 500。
- 图标尺寸 16x16，与文字间距 10。

- [ ] **Step 3: 修改 `DhModuleTabBar.qml` Tab 样式**

- 背景白、下边框 `#E5E9F0`、高度 56。
- Tab 按钮高度 36、圆角 8、padding 水平 16。
- 选中背景 `#2563EB`、白色文字；未选中白底 `#6B7280` 文字。

- [ ] **Step 4: 提交**

```bash
git add src/qml/DeployHub/MainShell.qml src/qml/DeployHub/DhSidebar.qml src/qml/DeployHub/DhModuleTabBar.qml
git commit -m "feat(qml): apply Pixso tool-style layout to main shell, sidebar and module tabs"
```

---

## Task 4: 仪表盘去 Dashboard 化

**Files:**
- Modify: `src/ui/MainWindow.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/style.qss`（如需要新 ID）

**目标：** 移除 Hero 横幅、统计卡网格、快捷操作，改为「最近部署任务」卡片 + 顶部 Tab 过滤。

- [ ] **Step 1: 删除仪表盘 Dashboard 控件**

在 `MainWindow` 中删除/注释以下构建函数调用：
- `buildDashboardHero()`
- `buildDashboardStats()`
- `buildDashboardQuickActions()`

- [ ] **Step 2: 新增仪表盘内容区**

新增一个 `QFrame#dashboardContent`：
- 顶部放 `QWidget#moduleTabBar` + 几个 `QPushButton#moduleTabButton`（全部/成功/失败）。
- 下方放 `QFrame#contentPanel` 内嵌 `QTableView`/`QListWidget` 展示最近部署记录。

- [ ] **Step 3: 确保表格行高紧凑、卡片标签不截断**

对列表/表格中的 QLabel 设置：

```cpp
label->setWordWrap(false);
label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
```

- [ ] **Step 4: 提交**

```bash
git add src/ui/MainWindow.cpp src/ui/MainWindow.h
git commit -m "feat(ui): redesign dashboard as compact tool-style task list"
```

---

## Task 5: 项目管理页工具风格改造

**Files:**
- Modify: `src/ui/ProjectManagerWidget.cpp/h`
- Modify: `src/ui/ProjectDialog.cpp/h`

**目标：** 左侧项目列表 + 右侧详情卡片，按钮/输入框使用统一样式。

- [ ] **Step 1: 改造布局为左右分栏**

```cpp
auto *splitter = new QSplitter(Qt::Horizontal);
splitter->addWidget(m_projectList);   // 左侧 220px 宽度
splitter->addWidget(m_detailCard);    // 右侧卡片
```

- [ ] **Step 2: 右侧详情卡片化**

将项目详情放入 `QFrame` 并设置 `setObjectName("contentPanel")`。

- [ ] **Step 3: 对话框按钮样式**

在 `ProjectDialog` 中为确定/取消按钮设置：

```cpp
okButton->setObjectName("primaryButton");
cancelButton->setObjectName("secondaryButton");
```

- [ ] **Step 4: 修复项目名称 QLabel 截断**

```cpp
m_nameLabel->setWordWrap(false);
m_nameLabel->setMinimumWidth(120);
m_nameLabel->setToolTip(m_nameLabel->text());
```

- [ ] **Step 5: 提交**

```bash
git add src/ui/ProjectManagerWidget.cpp src/ui/ProjectManagerWidget.h src/ui/ProjectDialog.cpp src/ui/ProjectDialog.h
git commit -m "feat(ui): apply tool-style layout to project manager"
```

---

## Task 6: 服务器管理页工具风格改造

**Files:**
- Modify: `src/ui/ServerManagerWidget.cpp/h`
- Modify: `src/ui/ServerDialog.cpp/h`

**目标：** 服务器卡片表格、状态 pill、主按钮。

- [ ] **Step 1: 列表使用 contentPanel 卡片**

将服务器列表放入 `QFrame#contentPanel`。

- [ ] **Step 2: 状态标签使用 pill 样式**

```cpp
statusLabel->setObjectName("serviceInstanceStatusOk");   // 成功
statusLabel->setObjectName("serviceInstanceStatusBad");  // 失败
```

- [ ] **Step 3: 连接测试按钮使用主按钮样式**

```cpp
testButton->setObjectName("primaryButton");
```

- [ ] **Step 4: 提交**

```bash
git add src/ui/ServerManagerWidget.cpp src/ui/ServerManagerWidget.h src/ui/ServerDialog.cpp src/ui/ServerDialog.h
git commit -m "feat(ui): apply tool-style layout to server manager"
```

---

## Task 7: 部署页工具风格改造

**Files:**
- Modify: `src/ui/DeployWorker.cpp/h`（如包含 UI 构建）
- Modify: `src/ui/MainWindow.cpp`（如部署页在主窗口中）

**目标：** 部署流程表单卡片化，进度与日志在同一页展示。

- [ ] **Step 1: 将部署选择表单放入卡片**

新建 `QFrame#deployFormCard`，内含项目选择、服务器选择、部署参数。

- [ ] **Step 2: 部署按钮使用主按钮样式**

```cpp
m_deployButton->setObjectName("primaryButton");
```

- [ ] **Step 3: 日志/进度面板改为右侧或底部卡片**

使用 `QFrame#contentPanel` 包裹日志查看器，避免弹窗。

- [ ] **Step 4: 提交**

```bash
git add src/ui/DeployWorker.cpp src/ui/DeployWorker.h src/ui/MainWindow.cpp
git commit -m "feat(ui): redesign deployment page as tool-style form + log cards"
```

---

## Task 8: 日志与 AI 分析页工具风格改造

**Files:**
- Modify: `src/ui/LogAiAnalysisWidget.cpp/h`
- Modify: `src/ui/DeploymentLogDialog.cpp/h`（如需要）

**目标：** 日志列表与详情分栏，AI 聊天气泡参考 Pixso 风格。

- [ ] **Step 1: 日志列表与详情分栏**

```cpp
auto *splitter = new QSplitter(Qt::Horizontal);
splitter->addWidget(m_logList);
splitter->addWidget(m_logDetailCard); // QFrame#contentPanel
```

- [ ] **Step 2: AI 聊天气泡样式**

```qss
QLabel#botMessage {
    background-color: @C24@;  /* 需在 Theme.h 增加 BotBubble="#F3F4F6" */
    border-radius: 0px 16px 16px 16px;
    padding: 12px 16px;
    color: @C2@;
}
QLabel#userMessage {
    background-color: @C6@;
    border-radius: 16px 0px 16px 16px;
    padding: 12px 16px;
    color: #FFFFFF;
}
```

如需新增 `@C24@`，在 `AppStyle.cpp` 增加：

```cpp
{QStringLiteral("@C24@"), QStringLiteral("#F3F4F6")},
```

- [ ] **Step 3: 提交**

```bash
git add src/ui/LogAiAnalysisWidget.cpp src/ui/LogAiAnalysisWidget.h src/ui/style.qss src/ui/AppStyle.cpp
git commit -m "feat(ui): redesign log and AI analysis page with tool-style chat bubbles"
```

---

## Task 9: 全局修复 Card QLabel 显示不全

**Files:**
- Modify: 所有使用 `dashboardTitle`、`serviceInstanceTitle`、`serviceParentTag`、`dashboardStatLabel` 等标题 QLabel 的文件。

**目标：** 避免标签被图标/按钮挤压导致截断。

- [ ] **Step 1: 在 style.qss 中为标题 QLabel 增加最小宽度与省略**

```qss
QLabel#dashboardTitle,
QLabel#serviceInstanceTitle,
QLabel#serviceParentTag,
QLabel#dashboardStatLabel {
    min-width: 60px;
}
```

- [ ] **Step 2: 在代码中为动态标题设置 `setWordWrap(false)` 和 tooltip**

```cpp
void setupCardLabel(QLabel *label) {
    if (!label) return;
    label->setWordWrap(false);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    label->setToolTip(label->text());
}
```

- [ ] **Step 3: 提交**

```bash
git add src/ui/style.qss src/ui/*.cpp src/ui/*.h
git commit -m "fix(ui): prevent card labels from being truncated"
```

---

## Task 10: 构建与测试验证

**Files:**
- 运行构建脚本与 CTest。

- [ ] **Step 1: 运行 Release 构建**

```bash
.\scripts\build-release.ps1
```

Expected: `deploy-hub.exe` 链接成功，无编译错误。

- [ ] **Step 2: 运行单元测试**

```bash
& D:\systemInsall\Qt\Tools\CMake_64\bin\ctest.exe --test-dir build-release --output-on-failure
```

Expected: 100% tests passed。

- [ ] **Step 3: 提交（如仅构建产物变化则忽略）**

```bash
git status
```

---

## 验收标准

- [ ] 左侧导航栏 220px，白底，选中项 `#EFF6FF` 背景 + `#2563EB` 文字。
- [ ] 顶部 Tab 栏高度 56px，选中 Tab 为蓝底白字圆角按钮。
- [ ] 内容区背景 `#F0F4F8`，卡片白底 + 12px 圆角 + 统一阴影。
- [ ] 主按钮高度 36px、圆角 8px、背景 `#2563EB`。
- [ ] 仪表盘不再出现 Hero/统计卡/快捷操作，改为紧凑任务列表。
- [ ] 项目管理、服务器、部署、日志均使用卡片 + 表格/表单布局。
- [ ] 所有 Card 标题/标签不被截断，hover 可查看 tooltip。
- [ ] `scripts/build-release.ps1` 与 `ctest` 通过。
