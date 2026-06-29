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
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QIcon makeAiConfigIcon(const QString &kind, const QColor &color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    if (kind == QStringLiteral("save")) {
        painter.drawRoundedRect(QRectF(3, 2.5, 10, 11), 2, 2);
        painter.drawLine(QPointF(5, 4.5), QPointF(10.5, 4.5));
        painter.drawRoundedRect(QRectF(5, 9, 6, 3.5), 1, 1);
    } else if (kind == QStringLiteral("test")) {
        painter.drawEllipse(QPointF(8, 8), 5, 5);
        painter.drawLine(QPointF(8, 5), QPointF(8, 8));
        painter.drawLine(QPointF(8, 8), QPointF(10.5, 9.5));
    } else if (kind == QStringLiteral("arrow")) {
        painter.drawLine(QPointF(5.5, 3.5), QPointF(10.5, 8));
        painter.drawLine(QPointF(10.5, 8), QPointF(5.5, 12.5));
    } else if (kind == QStringLiteral("bulb")) {
        painter.drawEllipse(QRectF(5, 2.5, 6, 7));
        painter.drawLine(QPointF(6.5, 10.5), QPointF(9.5, 10.5));
        painter.drawLine(QPointF(7, 13), QPointF(9, 13));
        painter.drawLine(QPointF(8, 9.5), QPointF(8, 11.5));
    }

    painter.end();
    return QIcon(pixmap);
}

QPushButton *makeAiConfigActionButton(const QString &text,
                                      const QString &iconKind,
                                      const QColor &iconColor,
                                      const QString &objectName,
                                      QWidget *parent)
{
    auto *button = new QPushButton(parent);
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setFixedHeight(32);
    auto *buttonLayout = new QHBoxLayout(button);
    buttonLayout->setContentsMargins(10, 0, 12, 0);
    buttonLayout->setSpacing(6);
    auto *iconLabel = new QLabel(button);
    iconLabel->setPixmap(makeAiConfigIcon(iconKind, iconColor).pixmap(14, 14));
    iconLabel->setFixedSize(14, 14);
    iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *textLabel = new QLabel(text, button);
    textLabel->setObjectName(QStringLiteral("aiConfigActionLabel"));
    textLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    buttonLayout->addWidget(iconLabel);
    buttonLayout->addWidget(textLabel);
    return button;
}

} // namespace

namespace Ui {
namespace Tools {

AiConfigToolPage::AiConfigToolPage(AiSettingsStore *aiSettings,
                                   CredentialStore *credentials,
                                   QWidget *parent)
    : ToolPage(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
{
    constexpr int kFormLabelWidth = 124;

    setObjectName(QStringLiteral("aiConfigPage"));
    setProperty("cardStackPage", true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space16);

    auto *formPanel = new QFrame(this);
    formPanel->setObjectName(QStringLiteral("aiConfigSection"));
    formPanel->setAttribute(Qt::WA_StyledBackground, true);
    formPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    PageLayout::applyLighterCardShadow(formPanel);
    auto *formPanelLayout = new QVBoxLayout(formPanel);
    formPanelLayout->setContentsMargins(PageLayout::Space24, PageLayout::Space16, PageLayout::Space24, PageLayout::Space16);
    formPanelLayout->setSpacing(PageLayout::Space12);

    formPanelLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("连接配置"), formPanel));

    auto makeFieldLabel = [kFormLabelWidth](const QString &text, QWidget *parent) {
        auto *label = new QLabel(text, parent);
        label->setObjectName(QStringLiteral("formFieldLabel"));
        label->setFixedWidth(kFormLabelWidth);
        label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        return label;
    };
    auto makeHintLabel = [](const QString &text, QWidget *parent) {
        auto *label = new QLabel(text, parent);
        label->setObjectName(QStringLiteral("formFieldHint"));
        return label;
    };
    auto makeFormRow = [makeFieldLabel, kFormLabelWidth](const QString &labelText,
                                                         QWidget *field,
                                                         QLabel *hint,
                                                         QWidget *parent) {
        auto *row = new QWidget(parent);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(PageLayout::Space16);
        rowLayout->addWidget(makeFieldLabel(labelText, row), 0, Qt::AlignTop);
        auto *fieldBlock = new QWidget(row);
        auto *fieldLayout = new QVBoxLayout(fieldBlock);
        fieldLayout->setContentsMargins(0, 0, 0, 0);
        fieldLayout->setSpacing(PageLayout::Space4);
        fieldLayout->addWidget(field);
        if (hint != nullptr) {
            fieldLayout->addWidget(hint);
        }
        rowLayout->addWidget(fieldBlock, 1);
        return row;
    };

    auto *apiBaseUrlInput = new QLineEdit(formPanel);
    apiBaseUrlInput->setPlaceholderText(QStringLiteral("https://api.openai.com/v1"));
    PageLayout::configureFormInput(apiBaseUrlInput);
    formPanelLayout->addWidget(makeFormRow(QStringLiteral("API Base URL"),
                                           apiBaseUrlInput,
                                           makeHintLabel(QStringLiteral("例如：https://api.openai.com/v1"), formPanel),
                                           formPanel));

    auto *passwordRow = new QWidget(formPanel);
    PageLayout::configureHorizontalFormRow(passwordRow);
    auto *passwordRowLayout = new QHBoxLayout(passwordRow);
    passwordRowLayout->setContentsMargins(0, 0, 0, 0);
    passwordRowLayout->setSpacing(PageLayout::Space8);
    auto *apiKeyInput = new QLineEdit(passwordRow);
    apiKeyInput->setEchoMode(QLineEdit::Password);
    apiKeyInput->setPlaceholderText(QStringLiteral("••••••••••••••••"));
    PageLayout::configureFormInput(apiKeyInput);
    auto *visibilityButton = new QPushButton(QStringLiteral("显示"), passwordRow);
    visibilityButton->setObjectName(QStringLiteral("aiVisibilityButton"));
    visibilityButton->setCheckable(true);
    visibilityButton->setFixedSize(56, PageLayout::DialogFieldHeight);
    connect(visibilityButton, &QPushButton::toggled, this, [apiKeyInput, visibilityButton](bool visible) {
        apiKeyInput->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
        visibilityButton->setText(visible ? QStringLiteral("隐藏") : QStringLiteral("显示"));
    });
    passwordRowLayout->addWidget(apiKeyInput, 1);
    passwordRowLayout->addWidget(visibilityButton);
    formPanelLayout->addWidget(makeFormRow(QStringLiteral("API Key"),
                                           passwordRow,
                                           makeHintLabel(QStringLiteral("您的 API Key 将安全存储在本地，不会上传到任何服务器。"), formPanel),
                                           formPanel));

    auto *modelInput = new QComboBox(formPanel);
    modelInput->setEditable(true);
    modelInput->addItems({
        QStringLiteral("gpt-4o-mini"),
        QStringLiteral("gpt-4o"),
        QStringLiteral("Qwen/Qwen2-7B"),
        QStringLiteral("deepseek-chat")
    });
    modelInput->setPlaceholderText(QStringLiteral("gpt-4o-mini"));
    PageLayout::configureFormInput(modelInput);
    formPanelLayout->addWidget(makeFormRow(QStringLiteral("模型"),
                                           modelInput,
                                           makeHintLabel(QStringLiteral("选择要使用的 AI 模型"), formPanel),
                                           formPanel));

    auto *rememberKeyCheck = new QCheckBox(QStringLiteral("记住 Key"), formPanel);
    rememberKeyCheck->setChecked(true);
    auto *rememberBlock = new QWidget(formPanel);
    auto *rememberLayout = new QVBoxLayout(rememberBlock);
    rememberLayout->setContentsMargins(0, 0, 0, 0);
    rememberLayout->setSpacing(PageLayout::Space4);
    rememberLayout->addWidget(rememberKeyCheck);
    rememberLayout->addWidget(makeHintLabel(QStringLiteral("下次启动时自动填充已保存凭据。"), formPanel));
    auto *rememberRow = new QWidget(formPanel);
    auto *rememberRowLayout = new QHBoxLayout(rememberRow);
    rememberRowLayout->setContentsMargins(kFormLabelWidth + PageLayout::Space16, 0, 0, 0);
    rememberRowLayout->setSpacing(0);
    rememberRowLayout->addWidget(rememberBlock, 1);
    formPanelLayout->addWidget(rememberRow);

    auto *actions = new QWidget(formPanel);
    actions->setObjectName(QStringLiteral("aiConfigActions"));
    auto *actionsLayout = new QHBoxLayout(actions);
    actionsLayout->setContentsMargins(0, PageLayout::Space8, 0, 0);
    actionsLayout->setSpacing(PageLayout::Space12);
    auto *saveButton = makeAiConfigActionButton(QStringLiteral("保存配置"),
                                                QStringLiteral("save"),
                                                QColor(QStringLiteral("#FFFFFF")),
                                                QStringLiteral("aiConfigSaveButton"),
                                                actions);
    auto *testButton = makeAiConfigActionButton(QStringLiteral("测试连接"),
                                                QStringLiteral("test"),
                                                QColor(QStringLiteral("#374151")),
                                                QStringLiteral("aiConfigTestButton"),
                                                actions);
    actionsLayout->addWidget(saveButton, 0, Qt::AlignLeft);
    actionsLayout->addWidget(testButton, 0, Qt::AlignLeft);
    actionsLayout->addStretch();
    formPanelLayout->addWidget(actions);

    auto *message = new QLabel(formPanel);
    message->setObjectName(QStringLiteral("toolMessage"));
    message->setContentsMargins(0, PageLayout::Space4, 0, 0);
    formPanelLayout->addWidget(message);
    layout->addWidget(formPanel);

    auto *helpPanel = new QFrame(this);
    helpPanel->setObjectName(QStringLiteral("aiQuickHelpPanel"));
    helpPanel->setAttribute(Qt::WA_StyledBackground, true);
    helpPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    PageLayout::applyLighterCardShadow(helpPanel);
    auto *helpLayout = new QVBoxLayout(helpPanel);
    helpLayout->setContentsMargins(PageLayout::Space24, PageLayout::Space16, PageLayout::Space24, PageLayout::Space16);
    helpLayout->setSpacing(PageLayout::Space12);
    helpLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("快速开始"), helpPanel));

    auto *steps = new QWidget(helpPanel);
    auto *stepsLayout = new QHBoxLayout(steps);
    stepsLayout->setContentsMargins(0, 0, 0, 0);
    stepsLayout->setSpacing(PageLayout::Space16);
    struct HelpStep {
        QString number;
        QString title;
        QString subtitle;
        QString link;
    };
    const QList<HelpStep> helpItems = {
        {QStringLiteral("1"), QStringLiteral("获取 API Key"), QStringLiteral("从 OpenAI 兼容服务商获取访问凭据"), QStringLiteral("查看文档 →")},
        {QStringLiteral("2"), QStringLiteral("配置连接信息"), QStringLiteral("填写 Base URL、API Key 和模型名称"), QString()},
        {QStringLiteral("3"), QStringLiteral("开始使用"), QStringLiteral("在 AI 聊天、辅助分析等工具中使用"), QString()}
    };
    for (int i = 0; i < helpItems.size(); ++i) {
        const HelpStep &item = helpItems.at(i);
        auto *step = new QWidget(steps);
        step->setObjectName(QStringLiteral("quickHelpStep"));
        auto *stepLayout = new QHBoxLayout(step);
        stepLayout->setContentsMargins(0, 0, 0, 0);
        stepLayout->setSpacing(PageLayout::Space12);
        auto *badge = new QLabel(item.number, step);
        badge->setObjectName(QStringLiteral("quickHelpBadge"));
        badge->setAlignment(Qt::AlignCenter);
        auto *textBlock = new QWidget(step);
        auto *textLayout = new QVBoxLayout(textBlock);
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(PageLayout::Space4);
        auto *title = new QLabel(item.title, textBlock);
        title->setObjectName(QStringLiteral("quickHelpTitle"));
        auto *subtitle = new QLabel(item.subtitle, textBlock);
        subtitle->setObjectName(QStringLiteral("quickHelpItem"));
        subtitle->setWordWrap(true);
        textLayout->addWidget(title);
        textLayout->addWidget(subtitle);
        if (!item.link.isEmpty()) {
            auto *link = new QLabel(item.link, textBlock);
            link->setObjectName(QStringLiteral("quickHelpLink"));
            textLayout->addWidget(link);
        }
        stepLayout->addWidget(badge);
        stepLayout->addWidget(textBlock, 1);
        stepsLayout->addWidget(step, 1);
        if (i + 1 < helpItems.size()) {
            auto *arrow = new QLabel(steps);
            arrow->setObjectName(QStringLiteral("quickHelpArrow"));
            arrow->setPixmap(makeAiConfigIcon(QStringLiteral("arrow"), QColor(QStringLiteral("#9CA3AF"))).pixmap(16, 16));
            arrow->setAlignment(Qt::AlignCenter);
            stepsLayout->addWidget(arrow);
        }
    }
    auto *bulb = new QLabel(steps);
    bulb->setObjectName(QStringLiteral("quickHelpBulb"));
    bulb->setPixmap(makeAiConfigIcon(QStringLiteral("bulb"), QColor(QStringLiteral("#4E8EFA"))).pixmap(20, 20));
    bulb->setAlignment(Qt::AlignCenter);
    bulb->setToolTip(QStringLiteral("API Key 保存在本地凭据存储中，不上传服务器。"));
    stepsLayout->addWidget(bulb, 0, Qt::AlignTop);
    helpLayout->addWidget(steps);
    layout->addWidget(helpPanel);

    auto *testClient = new OpenAiChatClient(this);

    auto loadForm = [m_aiSettings = m_aiSettings, m_credentials = m_credentials, apiBaseUrlInput, apiKeyInput, modelInput, rememberKeyCheck, message]() {
        if (m_aiSettings == nullptr) {
            return;
        }
        AiSettings settings;
        QString error;
        if (!m_aiSettings->load(&settings, &error)) {
            message->setText(error);
            return;
        }
        apiBaseUrlInput->setText(settings.apiBaseUrl);
        if (!settings.model.isEmpty()) {
            const int modelIndex = modelInput->findText(settings.model);
            if (modelIndex >= 0) {
                modelInput->setCurrentIndex(modelIndex);
            } else {
                modelInput->setEditText(settings.model);
            }
        }
        rememberKeyCheck->setChecked(settings.rememberKey);
        apiKeyInput->clear();
        apiKeyInput->setPlaceholderText(QStringLiteral("••••••••••••••••"));
        if (m_credentials != nullptr && m_credentials->has(settings.credentialRef)) {
            apiKeyInput->setText(m_credentials->load(settings.credentialRef));
        }
        message->clear();
    };
    loadForm();

    connect(saveButton, &QPushButton::clicked, this, [this, m_aiSettings = m_aiSettings, m_credentials = m_credentials, apiBaseUrlInput, apiKeyInput, modelInput, rememberKeyCheck, message, loadForm]() {
        if (m_aiSettings == nullptr || m_credentials == nullptr) {
            message->setText(QStringLiteral("AI 配置存储未初始化"));
            return;
        }

        AiSettings settings;
        QString loadError;
        m_aiSettings->load(&settings, &loadError);

        settings.apiBaseUrl = apiBaseUrlInput->text().trimmed();
        settings.model = modelInput->currentText().trimmed();
        settings.rememberKey = rememberKeyCheck->isChecked();
        if (settings.credentialRef.isEmpty()) {
            settings.credentialRef = QStringLiteral("deploy-hub/ai-api-key");
        }

        if (settings.apiBaseUrl.isEmpty()) {
            message->setText(QStringLiteral("请填写 API Base URL"));
            return;
        }
        if (settings.model.isEmpty()) {
            message->setText(QStringLiteral("请填写模型名称"));
            return;
        }

        QString saveError;
        if (!m_aiSettings->save(settings, &saveError)) {
            message->setText(saveError);
            return;
        }

        if (settings.rememberKey) {
            if (!apiKeyInput->text().isEmpty()) {
                if (!m_credentials->save(settings.credentialRef, apiKeyInput->text(), &saveError)) {
                    message->setText(saveError);
                    return;
                }
            } else if (!m_credentials->has(settings.credentialRef)) {
                message->setText(QStringLiteral("勾选记住 Key 时请填写 API Key"));
                return;
            }
        } else {
            m_credentials->remove(settings.credentialRef, &saveError);
        }

        message->setText(QStringLiteral("配置已保存"));
        loadForm();
    });

    connect(testButton, &QPushButton::clicked, this, [this, m_aiSettings = m_aiSettings, m_credentials = m_credentials, apiBaseUrlInput, apiKeyInput, modelInput, message, testClient, saveButton, testButton]() {
        AiSettings settings;
        settings.apiBaseUrl = apiBaseUrlInput->text().trimmed();
        settings.model = modelInput->currentText().trimmed();
        settings.credentialRef = QStringLiteral("deploy-hub/ai-api-key");

        QString key = apiKeyInput->text();
        if (key.isEmpty() && m_credentials != nullptr) {
            AiSettings stored;
            m_aiSettings->load(&stored, nullptr);
            key = m_credentials->load(stored.credentialRef);
        }

        QString error;
        if (settings.apiBaseUrl.isEmpty() || settings.model.isEmpty() || key.isEmpty()) {
            message->setText(QStringLiteral("请先填写 URL、模型和 API Key"));
            return;
        }

        testClient->abort();
        saveButton->setEnabled(false);
        testButton->setEnabled(false);
        message->setText(QStringLiteral("正在测试连接..."));

        connect(testClient, &OpenAiChatClient::finished, this, [message, testButton, saveButton]() {
            message->setText(QStringLiteral("连接成功"));
            testButton->setEnabled(true);
            saveButton->setEnabled(true);
        }, Qt::SingleShotConnection);
        connect(testClient, &OpenAiChatClient::failed, this, [message, testButton, saveButton](const QString &failedMessage) {
            message->setText(QStringLiteral("连接失败：%1").arg(failedMessage));
            testButton->setEnabled(true);
            saveButton->setEnabled(true);
        }, Qt::SingleShotConnection);

        testClient->testConnection(settings.apiBaseUrl, key, settings.model);
    });
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
