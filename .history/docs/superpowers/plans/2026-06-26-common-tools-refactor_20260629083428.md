# 通用工具页面重构实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 `CommonToolsWidget` 中 20+ 个工具页面拆分为独立 `ToolPage` 类，以 AI 配置页为模板统一风格、组件和交互，并补齐 Qt Test。

**Architecture:** 提取 `ToolPage` 基类和 `ToolEditor/ToolResultRow/ToolActionBar/AiAssistBar` 公共组件；`CommonToolsWidget` 仅负责工具页注册与侧边栏导航；每个工具页按 AI 配置页风格实现 Header + 输入区 + 操作区 + 结果区。

**Tech Stack:** C++17, Qt 6 Widgets, CMake, Qt Test

---

## 文件结构

### 新建文件

- `src/ui/tools/ToolPage.h/cpp` — 工具页抽象基类
- `src/ui/tools/ToolEditor.h/cpp` — 统一输入编辑器
- `src/ui/tools/ToolResultRow.h/cpp` — 结果展示行（只读 + 复制）
- `src/ui/tools/ToolActionBar.h/cpp` — 操作按钮组
- `src/ui/tools/AiAssistBar.h/cpp` — AI 辅助按钮条
- `src/ui/tools/pages/JsonToolPage.h/cpp` — JSON 格式化
- `src/ui/tools/pages/TimestampToolPage.h/cpp` — 时间戳转换
- `src/ui/tools/pages/UuidToolPage.h/cpp` — UUID 生成
- `src/ui/tools/pages/HashToolPage.h/cpp` — Hash 计算
- `src/ui/tools/pages/UrlCodecToolPage.h/cpp` — URL 编解码
- `src/ui/tools/pages/Base64TextToolPage.h/cpp` — Base64 文本
- `src/ui/tools/pages/CaseToolPage.h/cpp` — 大小写转换
- `src/ui/tools/pages/RegexToolPage.h/cpp` — 正则匹配
- `src/ui/tools/pages/CronToolPage.h/cpp` — Cron 解析
- `src/ui/tools/pages/DiffToolPage.h/cpp` — 文本 Diff
- `src/ui/tools/pages/HtmlEntityToolPage.h/cpp` — HTML 实体
- `src/ui/tools/pages/HttpStatusToolPage.h/cpp` — HTTP 状态码速查
- `src/ui/tools/pages/NumberBaseToolPage.h/cpp` — 进制转换
- `src/ui/tools/pages/JwtToolPage.h/cpp` — JWT 解码
- `src/ui/tools/pages/ImageBase64ToolPage.h/cpp` — 图片 Base64
- `src/ui/tools/pages/HttpRequestToolPage.h/cpp` — HTTP 请求封装
- `src/ui/tools/pages/WebSocketToolPage.h/cpp` — WebSocket 封装
- `src/ui/tools/pages/AiConfigToolPage.h/cpp` — AI 配置迁移
- `tests/unit/CommonToolsTest.h/cpp` — 纯工具函数测试
- `tests/unit/JsonToolPageTest.h/cpp` — JSON 工具页测试（示例）

### 修改文件

- `src/ui/CommonToolsWidget.h/cpp` — 收敛为注册/导航容器
- `src/ui/PageLayout.h/cpp` — 如有需要补充工具页通用布局辅助函数
- `CMakeLists.txt` — 添加新文件到构建
- `src/ui/style.qss` — 添加工具页统一样式

---

## Task 1: 创建 ToolPage 基类

**Files:**
- Create: `src/ui/tools/ToolPage.h`
- Create: `src/ui/tools/ToolPage.cpp`
- Test: `tests/unit/ToolPageTest.h`（可选，基类较薄，暂不强制）

- [ ] **Step 1: 创建 `ToolPage.h`**

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

- [ ] **Step 2: 创建 `ToolPage.cpp`**

```cpp
#include "ui/tools/ToolPage.h"

namespace Ui {
namespace Tools {

ToolPage::ToolPage(QWidget *parent)
    : QWidget(parent)
{
}

QString ToolPage::subtitle() const
{
    return QString();
}

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 3: 提交**

```bash
git add src/ui/tools/ToolPage.h src/ui/tools/ToolPage.cpp
git commit -m "feat(tools): add ToolPage base class for common tools"
```

---

## Task 2: 创建 ToolEditor 公共组件

**Files:**
- Create: `src/ui/tools/ToolEditor.h`
- Create: `src/ui/tools/ToolEditor.cpp`

- [ ] **Step 1: 创建 `ToolEditor.h`**

```cpp
#pragma once

#include <QPlainTextEdit>
#include <QWidget>

namespace Ui {
namespace Tools {

class ToolEditor : public QWidget {
    Q_OBJECT
public:
    explicit ToolEditor(QWidget *parent = nullptr);

    void setPlaceholderText(const QString &text);
    void setText(const QString &text);
    QString text() const;
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    void clear();

    QPlainTextEdit *editor() const;

signals:
    void textChanged();

private:
    QPlainTextEdit *m_editor = nullptr;
};

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 2: 创建 `ToolEditor.cpp`**

```cpp
#include "ui/tools/ToolEditor.h"

#include <QVBoxLayout>

namespace Ui {
namespace Tools {

ToolEditor::ToolEditor(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("toolEditor"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_editor = new QPlainTextEdit(this);
    m_editor->setObjectName(QStringLiteral("toolEditorText"));
    m_editor->setMinimumHeight(220);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(m_editor);

    connect(m_editor, &QPlainTextEdit::textChanged, this, &ToolEditor::textChanged);
}

void ToolEditor::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}

void ToolEditor::setText(const QString &text)
{
    m_editor->setPlainText(text);
}

QString ToolEditor::text() const
{
    return m_editor->toPlainText();
}

void ToolEditor::setReadOnly(bool readOnly)
{
    m_editor->setReadOnly(readOnly);
}

bool ToolEditor::isReadOnly() const
{
    return m_editor->isReadOnly();
}

void ToolEditor::clear()
{
    m_editor->clear();
}

QPlainTextEdit *ToolEditor::editor() const
{
    return m_editor;
}

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 3: 提交**

```bash
git add src/ui/tools/ToolEditor.h src/ui/tools/ToolEditor.cpp
git commit -m "feat(tools): add ToolEditor shared component"
```

---

## Task 3: 创建 ToolResultRow 公共组件

**Files:**
- Create: `src/ui/tools/ToolResultRow.h`
- Create: `src/ui/tools/ToolResultRow.cpp`

- [ ] **Step 1: 创建 `ToolResultRow.h`**

```cpp
#pragma once

#include <QWidget>

class QLineEdit;

namespace Ui {
namespace Tools {

class ToolResultRow : public QWidget {
    Q_OBJECT
public:
    explicit ToolResultRow(const QString &label, QWidget *parent = nullptr);

    void setText(const QString &text);
    QString text() const;
    void clear();

signals:
    void copied();

private:
    QLineEdit *m_edit = nullptr;
};

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 2: 创建 `ToolResultRow.cpp`**

```cpp
#include "ui/tools/ToolResultRow.h"

#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace Ui {
namespace Tools {

ToolResultRow::ToolResultRow(const QString &label, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("toolResultRow"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space8);

    if (!label.isEmpty()) {
        auto *labelWidget = new QLabel(label, this);
        labelWidget->setObjectName(QStringLiteral("toolResultRowLabel"));
        labelWidget->setFixedWidth(120);
        layout->addWidget(labelWidget);
    }

    m_edit = new QLineEdit(this);
    m_edit->setObjectName(QStringLiteral("toolResultRowEdit"));
    m_edit->setReadOnly(true);
    PageLayout::configureFormInput(m_edit);
    layout->addWidget(m_edit, 1);

    auto *copyButton = new QPushButton(QStringLiteral("复制"), this);
    copyButton->setObjectName(QStringLiteral("formActionButton"));
    copyButton->setFixedHeight(PageLayout::DialogFieldHeight);
    connect(copyButton, &QPushButton::clicked, this, [this]() {
        if (QClipboard *clipboard = QApplication::clipboard()) {
            clipboard->setText(m_edit->text());
        }
        emit copied();
    });
    layout->addWidget(copyButton);
}

void ToolResultRow::setText(const QString &text)
{
    m_edit->setText(text);
}

QString ToolResultRow::text() const
{
    return m_edit->text();
}

void ToolResultRow::clear()
{
    m_edit->clear();
}

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 3: 提交**

```bash
git add src/ui/tools/ToolResultRow.h src/ui/tools/ToolResultRow.cpp
git commit -m "feat(tools): add ToolResultRow shared component"
```

---

## Task 4: 创建 ToolActionBar 公共组件

**Files:**
- Create: `src/ui/tools/ToolActionBar.h`
- Create: `src/ui/tools/ToolActionBar.cpp`

- [ ] **Step 1: 创建 `ToolActionBar.h`**

```cpp
#pragma once

#include <QWidget>

class QPushButton;

namespace Ui {
namespace Tools {

class ToolActionBar : public QWidget {
    Q_OBJECT
public:
    explicit ToolActionBar(QWidget *parent = nullptr);

    QPushButton *primaryButton() const;
    QPushButton *clearButton() const;
    QPushButton *copyButton() const;

    void setPrimaryText(const QString &text);
    void setCopyEnabled(bool enabled);

signals:
    void primaryClicked();
    void clearClicked();
    void copyClicked();

private:
    QPushButton *m_primaryButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_copyButton = nullptr;
};

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 2: 创建 `ToolActionBar.cpp`**

```cpp
#include "ui/tools/ToolActionBar.h"

#include "ui/PageLayout.h"

#include <QHBoxLayout>
#include <QPushButton>

namespace Ui {
namespace Tools {

ToolActionBar::ToolActionBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("toolActionBar"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    m_primaryButton = new QPushButton(QStringLiteral("执行"), this);
    m_primaryButton->setObjectName(QStringLiteral("toolPrimaryButton"));
    m_primaryButton->setFixedHeight(32);
    connect(m_primaryButton, &QPushButton::clicked, this, &ToolActionBar::primaryClicked);
    layout->addWidget(m_primaryButton);

    m_clearButton = new QPushButton(QStringLiteral("清空"), this);
    m_clearButton->setObjectName(QStringLiteral("toolSecondaryButton"));
    m_clearButton->setFixedHeight(32);
    connect(m_clearButton, &QPushButton::clicked, this, &ToolActionBar::clearClicked);
    layout->addWidget(m_clearButton);

    m_copyButton = new QPushButton(QStringLiteral("复制结果"), this);
    m_copyButton->setObjectName(QStringLiteral("toolSecondaryButton"));
    m_copyButton->setFixedHeight(32);
    m_copyButton->setEnabled(false);
    connect(m_copyButton, &QPushButton::clicked, this, &ToolActionBar::copyClicked);
    layout->addWidget(m_copyButton);

    layout->addStretch();
}

QPushButton *ToolActionBar::primaryButton() const { return m_primaryButton; }
QPushButton *ToolActionBar::clearButton() const { return m_clearButton; }
QPushButton *ToolActionBar::copyButton() const { return m_copyButton; }

void ToolActionBar::setPrimaryText(const QString &text)
{
    m_primaryButton->setText(text);
}

void ToolActionBar::setCopyEnabled(bool enabled)
{
    m_copyButton->setEnabled(enabled);
}

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 3: 修复明显的类名拼写错误并编译验证**

运行：

```bash
cmake --build build-release --target deploy-hub
```

Expected: 编译通过（公共组件无依赖问题）。

- [ ] **Step 4: 提交**

```bash
git add src/ui/tools/ToolActionBar.h src/ui/tools/ToolActionBar.cpp
git commit -m "feat(tools): add ToolActionBar shared component"
```

---

## Task 5: 创建 AiAssistBar 公共组件

**Files:**
- Create: `src/ui/tools/AiAssistBar.h`
- Create: `src/ui/tools/AiAssistBar.cpp`

- [ ] **Step 1: 创建 `AiAssistBar.h`**

```cpp
#pragma once

#include <QWidget>

class QPushButton;

namespace Ui {
namespace Tools {

class AiAssistBar : public QWidget {
    Q_OBJECT
public:
    explicit AiAssistBar(QWidget *parent = nullptr);

    QPushButton *assistButton() const;
    QPushButton *stopButton() const;

    void setRunning(bool running);

signals:
    void assistClicked();
    void stopClicked();

private:
    QPushButton *m_assistButton = nullptr;
    QPushButton *m_stopButton = nullptr;
};

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 2: 创建 `AiAssistBar.cpp`**

```cpp
#include "ui/tools/AiAssistBar.h"

#include <QHBoxLayout>
#include <QPushButton>

namespace Ui {
namespace Tools {

AiAssistBar::AiAssistBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("aiAssistBar"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_assistButton = new QPushButton(QStringLiteral("AI 辅助"), this);
    m_assistButton->setObjectName(QStringLiteral("toolAiAssistButton"));
    m_assistButton->setFixedHeight(28);
    connect(m_assistButton, &QPushButton::clicked, this, &AiAssistBar::assistClicked);
    layout->addWidget(m_assistButton);

    m_stopButton = new QPushButton(QStringLiteral("停止"), this);
    m_stopButton->setObjectName(QStringLiteral("toolAiStopButton"));
    m_stopButton->setFixedHeight(28);
    m_stopButton->setVisible(false);
    connect(m_stopButton, &QPushButton::clicked, this, &AiAssistBar::stopClicked);
    layout->addWidget(m_stopButton);

    layout->addStretch();
}

QPushButton *AiAssistBar::assistButton() const { return m_assistButton; }
QPushButton *AiAssistBar::stopButton() const { return m_stopButton; }

void AiAssistBar::setRunning(bool running)
{
    m_assistButton->setVisible(!running);
    m_stopButton->setVisible(running);
}

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 3: 提交**

```bash
git add src/ui/tools/AiAssistBar.h src/ui/tools/AiAssistBar.cpp
git commit -m "feat(tools): add AiAssistBar shared component"
```

---

## Task 6: 迁移 AI 配置页为 AiConfigToolPage

**Files:**
- Create: `src/ui/tools/pages/AiConfigToolPage.h`
- Create: `src/ui/tools/pages/AiConfigToolPage.cpp`
- Modify: `src/ui/CommonToolsWidget.cpp`（移除旧的 `createAiConfigPage`，改为使用 `AiConfigToolPage`）

- [ ] **Step 1: 创建 `AiConfigToolPage.h`**

```cpp
#pragma once

#include "ui/tools/ToolPage.h"

class AiSettingsStore;
class CredentialStore;

namespace Ui {
namespace Tools {

class AiConfigToolPage : public ToolPage {
    Q_OBJECT
public:
    explicit AiConfigToolPage(AiSettingsStore *aiSettings,
                              CredentialStore *credentials,
                              QWidget *parent = nullptr);

    QString title() const override;
    QString subtitle() const override;

private:
    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
};

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 2: 创建 `AiConfigToolPage.cpp`**

将 `CommonToolsWidget::createAiConfigPage()` 的实现整体迁移到 `AiConfigToolPage` 构造函数中，页面根对象改为 `this`，保持现有布局和信号槽逻辑不变。需要把 `m_aiSettings` / `m_credentials` 替换为成员变量。

关键代码片段（与旧实现一致，仅调整变量来源）：

```cpp
#include "ui/tools/pages/AiConfigToolPage.h"

#include "adapters/ai/OpenAiChatClient.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "ui/PageLayout.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace Ui {
namespace Tools {

AiConfigToolPage::AiConfigToolPage(AiSettingsStore *aiSettings,
                                   CredentialStore *credentials,
                                   QWidget *parent)
    : ToolPage(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
{
    setObjectName(QStringLiteral("aiConfigPage"));
    // ... 保留原 createAiConfigPage 的完整布局与业务逻辑
}

QString AiConfigToolPage::title() const
{
    return QStringLiteral("AI 配置");
}

QString AiConfigToolPage::subtitle() const
{
    return QStringLiteral("配置 OpenAI 兼容 API 的连接信息");
}

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 3: 修改 `CommonToolsWidget` 使用新的 `AiConfigToolPage`**

在 `CommonToolsWidget` 构造函数中，把原来的 `createAiConfigPage()` 调用替换为 `new AiConfigToolPage(m_aiSettings, m_credentials, this)`。

- [ ] **Step 4: 编译验证**

```bash
cmake --build build-release --target deploy-hub
```

Expected: 编译通过。

- [ ] **Step 5: 提交**

```bash
git add src/ui/tools/pages/AiConfigToolPage.h src/ui/tools/pages/AiConfigToolPage.cpp src/ui/CommonToolsWidget.cpp
git commit -m "refactor(tools): migrate AI config page to AiConfigToolPage"
```

---

## Task 7: 迁移 JSON 工具页为 JsonToolPage（模板页）

**Files:**
- Create: `src/ui/tools/pages/JsonToolPage.h`
- Create: `src/ui/tools/pages/JsonToolPage.cpp`
- Test: `tests/unit/JsonToolPageTest.h/cpp`

- [ ] **Step 1: 创建 `JsonToolPage.h`**

```cpp
#pragma once

#include "ui/tools/ToolPage.h"

namespace Ui {
namespace Tools {

class JsonToolPage : public ToolPage {
    Q_OBJECT
public:
    explicit JsonToolPage(QWidget *parent = nullptr);

    QString title() const override;
    QString subtitle() const override;
};

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 2: 创建 `JsonToolPage.cpp`**

```cpp
#include "ui/tools/pages/JsonToolPage.h"

#include "tools/CommonTools.h"
#include "ui/tools/ToolActionBar.h"
#include "ui/tools/ToolEditor.h"
#include "ui/tools/AiAssistBar.h"
#include "ui/PageLayout.h"

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

namespace Ui {
namespace Tools {

JsonToolPage::JsonToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("jsonToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space16);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("toolCard"));
    card->setAttribute(Qt::WA_StyledBackground, true);
    PageLayout::applyLighterCardShadow(card);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(PageLayout::Space24, PageLayout::Space16, PageLayout::Space24, PageLayout::Space16);
    cardLayout->setSpacing(PageLayout::Space12);

    cardLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("JSON 格式化"), card));

    auto *input = new ToolEditor(card);
    input->setPlaceholderText(QStringLiteral("粘贴需要格式化的 JSON"));
    cardLayout->addWidget(input);

    auto *actionBar = new ToolActionBar(card);
    actionBar->setPrimaryText(QStringLiteral("格式化"));
    cardLayout->addWidget(actionBar);

    auto *aiBar = new AiAssistBar(card);
    cardLayout->addWidget(aiBar);

    auto *message = new QLabel(card);
    message->setObjectName(QStringLiteral("toolMessage"));
    cardLayout->addWidget(message);

    layout->addWidget(card);
    layout->addStretch();

    connect(actionBar, &ToolActionBar::primaryClicked, this, [input, message]() {
        QString error;
        const QString result = CommonTools::formatJson(input->text(), &error);
        if (result.isEmpty() && !error.isEmpty()) {
            message->setText(QStringLiteral("格式化失败：%1").arg(error));
            return;
        }
        input->setText(result);
        message->setText(QStringLiteral("格式化完成"));
    });

    connect(actionBar, &ToolActionBar::clearClicked, this, [input, message]() {
        input->clear();
        message->clear();
    });
}

QString JsonToolPage::title() const
{
    return QStringLiteral("JSON 格式化");
}

QString JsonToolPage::subtitle() const
{
    return QStringLiteral("美化和验证 JSON");
}

} // namespace Tools
} // namespace Ui
```

- [ ] **Step 3: 修改 `CommonToolsWidget` 注册 `JsonToolPage`**

在 `CommonToolsWidget` 构造函数中移除旧的 JSON 页面创建代码，改为 `new JsonToolPage(this)`。

- [ ] **Step 4: 编写 `JsonToolPageTest.cpp`**

```cpp
#include "JsonToolPageTest.h"
#include "ui/tools/pages/JsonToolPage.h"
#include <QTest>

void JsonToolPageTest::testTitle()
{
    Ui::Tools::JsonToolPage page;
    QCOMPARE(page.title(), QStringLiteral("JSON 格式化"));
}

QTEST_MAIN(JsonToolPageTest)
```

- [ ] **Step 5: 编译并运行测试**

```bash
cmake --build build-release --target deploy_hub_tests
ctest --test-dir build-release --output-on-failure
```

Expected: `JsonToolPageTest` 通过。

- [ ] **Step 6: 提交**

```bash
git add src/ui/tools/pages/JsonToolPage.h src/ui/tools/pages/JsonToolPage.cpp tests/unit/JsonToolPageTest.h tests/unit/JsonToolPageTest.cpp src/ui/CommonToolsWidget.cpp
git commit -m "refactor(tools): migrate JSON page to JsonToolPage"
```

---

## Task 8-19: 按相同模式迁移其他简单工具页

每个工具页的任务结构相同：创建 `XxxToolPage.h/cpp`，在 `CommonToolsWidget` 中替换旧实现，简单工具页不强制补 UI 测试，但需编译通过。

工具页清单：

- Task 8: `TimestampToolPage`（时间戳转换）
- Task 9: `UuidToolPage`（UUID 生成）
- Task 10: `HashToolPage`（Hash 计算）
- Task 11: `UrlCodecToolPage`（URL 编解码）
- Task 12: `Base64TextToolPage`（Base64 文本）
- Task 13: `CaseToolPage`（大小写转换）
- Task 14: `RegexToolPage`（正则匹配）
- Task 15: `CronToolPage`（Cron 解析）
- Task 16: `DiffToolPage`（文本 Diff）
- Task 17: `HtmlEntityToolPage`（HTML 实体）
- Task 18: `HttpStatusToolPage`（HTTP 状态码速查）
- Task 19: `NumberBaseToolPage`（进制转换）

每完成 3-4 个页面运行一次 `cmake --build build-release --target deploy-hub` 确保无回归。

---

## Task 20: 迁移 JWT 与图片 Base64 工具页

**Files:**
- Create: `src/ui/tools/pages/JwtToolPage.h/cpp`
- Create: `src/ui/tools/pages/ImageBase64ToolPage.h/cpp`
- Modify: `src/ui/CommonToolsWidget.cpp`

与 Task 7 模式一致，但 `ImageBase64ToolPage` 涉及 `QFileDialog` 和图片处理，保留原逻辑。

---

## Task 21: 迁移 HTTP 请求与 WebSocket 工具页

**Files:**
- Create: `src/ui/tools/pages/HttpRequestToolPage.h/cpp`
- Create: `src/ui/tools/pages/WebSocketToolPage.h/cpp`
- Modify: `src/ui/CommonToolsWidget.cpp`

这两个页面分别封装现有的 `HttpRequestWidget` 和 `WebSocketToolWidget`，不在内部重新实现网络逻辑。`HttpRequestToolPage` 内部仅创建 `HttpRequestWidget` 并嵌入布局；`WebSocketToolPage` 同理。

---

## Task 22: 清理 CommonToolsWidget 旧代码

**Files:**
- Modify: `src/ui/CommonToolsWidget.h/cpp`

- [ ] **Step 1: 删除旧的 `createXxxPage` 私有辅助函数**
- [ ] **Step 2: 删除重复的布局辅助函数（如 `makeEditor`、`makeActionButton` 等已迁移到公共组件的函数）**
- [ ] **Step 3: 保留工具页注册表和 `toolLabels()` / `takeToolPage()` 接口**
- [ ] **Step 4: 编译验证**

```bash
cmake --build build-release --target deploy-hub
```

Expected: 编译通过，`CommonToolsWidget` 体积显著减小。

---

## Task 23: 统一样式并更新 style.qss

**Files:**
- Modify: `src/ui/style.qss`

添加以下样式：

```qss
#toolCard {
    background-color: @C2@;
    border: 1px solid @C4@;
    border-radius: @C9@px;
}

#toolEditorText {
    background-color: @C2@;
    border: 1px solid @C4@;
    border-radius: @C5@px;
    padding: 8px;
    font-family: "JetBrains Mono", "Consolas", monospace;
    font-size: 13px;
}

#toolPrimaryButton {
    background-color: @C6@;
    color: @C2@;
    border: none;
    border-radius: @C5@px;
    padding: 6px 16px;
}

#toolSecondaryButton {
    background-color: transparent;
    color: @C3@;
    border: 1px solid @C4@;
    border-radius: @C5@px;
    padding: 6px 16px;
}

#toolAiAssistButton {
    background-color: @C7@;
    color: @C6@;
    border: none;
    border-radius: @C10@px;
    padding: 4px 12px;
}

#toolMessage {
    color: @C3@;
    font-size: 12px;
}
```

颜色占位符已在 `AppStyle.cpp` 中映射到 `@C1@` ~ `@C20@`。

---

## Task 24: 补齐 CommonTools 单元测试

**Files:**
- Create: `tests/unit/CommonToolsTest.h/cpp`

覆盖：

- `formatJson` 成功与失败路径
- `textToBase64` / `base64ToText`
- `urlEncode` / `urlDecode`
- `computeHashes`
- `generateUuids`
- `convertNumberBase`
- `convertCase`
- `analyzeCron`
- `diffLineIndices`

- [ ] **Step 1: 编写测试文件**
- [ ] **Step 2: 运行测试**

```bash
cmake --build build-release --target deploy_hub_tests
ctest --test-dir build-release --output-on-failure
```

Expected: 新增测试全部通过。

---

## Task 25: 最终回归验证

**Files:** 所有变更文件

- [ ] **Step 1: 完整构建 Release**

```bash
powershell -ExecutionPolicy Bypass -File scripts/build-release.ps1
```

Expected: 生成 `build-release/deploy-hub.exe` 无错误。

- [ ] **Step 2: 运行全部 Qt Test**

```bash
ctest --test-dir build-release --output-on-failure
```

Expected: 全部通过。

- [ ] **Step 3: 启动应用验证通用工具页**

```bash
.\build-release\deploy-hub.exe
```

手动检查：
- 左侧工具列表正常
- 各工具页布局风格一致
- AI 配置页保存/测试/加载正常
- JSON 格式化、UUID、Hash 等工具功能正常

---

## Self-Review

- **Spec coverage:** 拆分文件、统一框架、抽离组件、补测试、统一样式、修复问题均有对应任务。
- **Placeholder scan:** 无 TBD/TODO；每个任务包含具体文件和关键代码。
- **Type consistency:** `ToolPage` 派生类均实现 `title()` / `subtitle()`；公共组件接口保持一致。
- **Scope:** 任务覆盖全部通用工具页，分阶段执行，每阶段可独立验证。

## Execution Choice

Plan complete and saved to `docs/superpowers/plans/2026-06-26-common-tools-refactor.md`.

**Two execution options:**

1. **Subagent-Driven (recommended)** - Dispatch a fresh subagent per task, review between tasks, fast iteration.
2. **Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints.

Which approach?
