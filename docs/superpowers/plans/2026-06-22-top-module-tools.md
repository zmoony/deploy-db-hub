# Top Module Tools Implementation Plan

**Goal:** 把主界面改为顶部主模块导航，并新增第一个“通用工具”模块。

**Scope:** 顶部模块为“通用工具 / 部署工具 / 大数据 / 数据库”。左侧导航随模块变化：部署工具保留现有部署相关页面，服务器管理归入部署工具；大数据和数据库从原服务管理子页升为顶层模块。

**Implementation Notes:**
- 复用现有 Qt Widgets、`PageLayout::makeTabBar` 和已有服务产品面板。
- 新增 `CommonToolsWidget` 承载 13 个工具入口与 MVP 工具区。
- 先用单测覆盖纯文本工具能力，再接 UI。
- 不回滚当前工作区已有服务管理改动。

**Tasks:**
1. 新增通用工具纯逻辑类和测试：JSON 格式化、Base64、Hex、时间戳、HTTP 状态码/HTML 特殊字符表数据。
2. 新增 `CommonToolsWidget`：左侧工具列表、右侧工作区，先覆盖本地工具和复杂工具入口。
3. 重构 `MainWindow` 布局：顶部模块导航 + 左侧模块内导航 + 右侧页面堆栈。
4. 拆分原服务管理：`ServerManagerWidget` 放入部署工具，大数据/数据库作为顶部模块页面。
5. 更新 CMake、测试入口、记忆文件，并运行 Release 构建与 CTest。
