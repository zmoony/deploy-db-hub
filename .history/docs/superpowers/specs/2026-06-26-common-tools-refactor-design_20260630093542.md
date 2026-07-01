# 通用工具页面重构设计

## 背景

当前 `src/ui/CommonToolsWidget.h/cpp` 把 AI 配置、HTTP 请求、JSON 格式化、时间戳、UUID、Hash、URL 编解码、Base64、正则、Cron、Diff、HTML 实体、HTTP 状态码、进制转换、大小写转换、图片 Base64、JWT、WebSocket 等 20+ 个工具页面全部实现在一个 Widget 类中。随着工具数量增加，单文件体积过大，导致：

- 阅读和维护成本高
- UI 布局代码重复严重
- 新增工具页容易破坏既有页面
- 难以针对单个工具页写单元测试
- 样式不统一，部分页面与 JetBrains IDE 风格不一致

## 目标

1. 拆分文件：每个工具页拆分为独立类/文件
2. 统一框架：所有工具页采用一致的输入-输出-操作布局模板
3. 抽离组件：提取通用的编辑器、结果行、AI 辅助按钮、操作按钮组等
4. 补测试：为工具函数和核心 UI 行为补 Qt Test
5. UI 样式统一：所有通用工具页遵循 JetBrains IDE 风格
6. 修复已知问题：顺手修复拆分过程中发现的重复代码和交互不一致问题

## 方案对比

| 方案 | 描述 | 优点 | 缺点 |
|------|------|------|------|
| A. 大爆炸式重构 | 一次性把所有工具页拆分为独立类，并统一样式和测试 | 一次性完成，后续干净 | 改动巨大，回归风险高，review 困难 |
| B. 渐进式按批拆分 | 按工具类别分批拆分，每批独立设计、实现、测试 | 风险可控 | 周期长，中间状态 CommonToolsWidget 和独立类并存 |
| C. 先框架后迁移（推荐） | 先抽出 `ToolPage` 基类和公共组件，建立测试基线，再逐个迁移工具页 | 风险低，组件可复用，每步可独立验证 | 初期需要额外设计工作 |

本设计采用方案 C。

## 架构设计

### 目录结构

```text
src/
  ui/
    tools/
      ToolPage.h                      # 通用工具页抽象基类
      ToolPage.cpp
      ToolEditor.h                    # 统一输入编辑器
      ToolEditor.cpp
      ToolResultRow.h                 # 结果展示行（只读框 + 复制按钮）
      ToolResultRow.cpp
      ToolActionBar.h                 # 操作按钮组（主操作/清空/复制）
      ToolActionBar.cpp
      AiAssistBar.h                   # AI 辅助按钮条
      AiAssistBar.cpp
      CommonToolsWidget.h             # 职责收敛为注册和导航
      CommonToolsWidget.cpp
      pages/
        AiConfigToolPage.h            # AI 配置
        AiConfigToolPage.cpp
        JsonToolPage.h                # JSON 格式化
        JsonToolPage.cpp
        TimestampToolPage.h           # 时间戳转换
        TimestampToolPage.cpp
        UuidToolPage.h                # UUID 生成
        UuidToolPage.cpp
        HashToolPage.h                # Hash 计算
        HashToolPage.cpp
        UrlCodecToolPage.h            # URL 编解码
        UrlCodecToolPage.cpp
        Base64TextToolPage.h          # Base64 文本
        Base64TextToolPage.cpp
        CaseToolPage.h                # 大小写转换
        CaseToolPage.cpp
        RegexToolPage.h               # 正则匹配
        RegexToolPage.cpp
        CronToolPage.h                # Cron 解析
        CronToolPage.cpp
        DiffToolPage.h                # 文本 Diff
        DiffToolPage.cpp
        HtmlEntityToolPage.h          # HTML 实体
        HtmlEntityToolPage.cpp
        HttpStatusToolPage.h          # HTTP 状态码速查
        HttpStatusToolPage.cpp
        NumberBaseToolPage.h          # 进制转换
        NumberBaseToolPage.cpp
        JwtToolPage.h                 # JWT 解码
        JwtToolPage.cpp
        ImageBase64ToolPage.h         # 图片 Base64
        ImageBase64ToolPage.cpp
        HttpRequestToolPage.h         # HTTP 请求（封装现有 HttpRequestWidget）
        HttpRequestToolPage.cpp
        WebSocketToolPage.h           # WebSocket（封装现有 WebSocketToolWidget）
        WebSocketToolPage.cpp
```

### 公共基类

```cpp
#pragma once

#include <QWidget>

namespace Ui {
namespace Tools {

class ToolPage : public QWidget {
    Q_OBJECT
public:
    explicit ToolPage(QWidget *parent = nullptr);
    virtual ~ToolPage() = default;

    virtual QString title() const = 0;
    virtual QString subtitle() const;

signals:
    void statusMessage(const QString &text, int timeoutMs = 3000);
};

} // namespace Tools
} // namespace Ui
```

### 公共组件

- `ToolEditor`：统一输入区，支持多行/单行、placeholder、清空按钮
- `ToolResultRow`：统一结果区，只读 + 复制按钮
- `ToolActionBar`：统一操作按钮组（主操作、清空、复制结果）
- `AiAssistBar`：AI 辅助按钮条，可选显示停止按钮

### CommonToolsWidget 职责

重构后 `CommonToolsWidget` 只负责：

1. 创建所有 `ToolPage` 实例
2. 构建左侧工具列表和右侧内容栈
3. 处理工具切换
4. 通过 `toolLabels()` 和 `takeToolPage()` 与 `MainWindow` 交互（保持现有接口兼容）

## 阶段计划

| 阶段 | 内容 | 验证标准 |
|------|------|----------|
| 阶段 1 | 提取 `ToolPage` 基类、`ToolEditor`、`ToolResultRow`、`ToolActionBar`、`AiAssistBar` | 组件可独立编译，CommonToolsWidget 仍能运行 |
| 阶段 2 | 拆分 JSON / 时间戳 / UUID / Hash / URL / Base64 / 大小写 等简单工具页 | 每个页面独立运行，布局统一 |
| 阶段 3 | 拆分正则 / Cron / Diff / HTML 实体 / HTTP 状态码 / 进制 / JWT / 图片 Base64 等复杂工具页 | 功能与重构前一致 |
| 阶段 4 | 拆分 AI 配置页 | 配置保存/加载/测试正常 |
| 阶段 5 | 拆分 HTTP 请求 / WebSocket 页（复用现有 Widget） | 网络功能正常 |
| 阶段 6 | 统一样式、补测试、清理旧代码 | 全部 Qt Test 通过，构建成功 |

## 模板标准：AI 配置页

所有通用工具页以当前 **AI 配置页**为重构模板，具体包括：

- **代码组织方式**：Header（标题/副标题）+ 输入区 + 操作按钮 + 结果区
- **视觉样式照搬**：间距、圆角、按钮大小、字体大小、颜色用法等保持与 AI 配置页一致
- **保留 AI 辅助按钮**：每个工具页右上角/操作区保留 AI 辅助入口，点击可调起 AI 帮助
- **左侧列表形式不变**：`CommonToolsWidget` 保持现有侧边栏工具列表 + 右侧内容栈的导航结构，仅把每个子页面替换为符合模板的独立 `ToolPage`

## 样式规范

- 所有工具页对象名统一前缀 `toolPage`
- 编辑器使用 `ToolEditor` 样式类
- 结果区使用 `ToolResultRow` 样式类
- 按钮组使用 `ToolActionBar` 样式类
- AI 辅助按钮统一使用 `AiAssistBar` 组件
- 主题色通过 `style.qss` 中的 `@C1@` ~ `@C20@` 占位符统一替换
- 遵循 JetBrains IDE 风格： minimal shadows, thin borders, 4-8px rounded corners, high information density

## 测试策略

1. 纯工具函数：已在 `src/tools/CommonTools.h`，补 `tests/unit/CommonToolsTest.cpp`
2. 单个 `ToolPage` 行为：每个页面迁移后补充对应测试
3. 避免继续测试巨型 `CommonToolsWidget`
4. 每阶段完成后运行 `ctest` 和 `scripts/build-release.ps1` 验证

## 兼容性

- `CommonToolsWidget` 对 `MainWindow` 的接口保持兼容：
  - `toolLabels()` 返回 `QStringList`
  - `toolPageCount()` 返回 `int`
  - `takeToolPage(int index)` 返回 `QWidget*`
- 新增工具页只需在 `CommonToolsWidget` 注册表中添加一行，不影响其他页面

## 风险与回退

- 风险：迁移过程中遗漏信号/槽或布局细节
- 缓解：逐个迁移，每阶段编译验证；保留原文件提交历史以便回退
- 回退：如某页面迁移后异常，可临时恢复旧实现，独立修复
