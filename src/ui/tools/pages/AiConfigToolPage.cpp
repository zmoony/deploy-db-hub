#include "ui/tools/pages/AiConfigToolPage.h"

#include "adapters/ai/OpenAiChatClient.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "ui/PageLayout.h"
#include "ui/tools/pages/AiConnectionManagerDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
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
    } else if (kind == QStringLiteral("manager")) {
        painter.drawRoundedRect(QRectF(2, 3.5, 5, 9), 1, 1);
        painter.drawRoundedRect(QRectF(9, 3.5, 5, 9), 1, 1);
        painter.drawLine(QPointF(4, 6), QPointF(4.5, 6));
        painter.drawLine(QPointF(4, 8.5), QPointF(5, 8.5));
        painter.drawLine(QPointF(11, 6), QPointF(12, 6));
        painter.drawLine(QPointF(11, 8.5), QPointF(12, 8.5));
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

QStringList collectModelChoices(const QVector<AiConnection> &connections, const QString &fallback)
{
    QStringList models;
    auto addUnique = [&models](const QString &value) {
        const QString trimmed = value.trimmed();
        if (trimmed.isEmpty()) {
            return;
        }
        if (!models.contains(trimmed)) {
            models.append(trimmed);
        }
    };
    for (const QString &preset : AiSettingsStore::defaultModelPresets()) {
        addUnique(preset);
    }
    for (const AiConnection &connection : connections) {
        addUnique(connection.model);
    }
    addUnique(fallback);
    return models;
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
    buildUi();
    loadForm();
}

void AiConfigToolPage::buildUi()
{
    constexpr int kFormLabelWidth = 124;

    setObjectName(QStringLiteral("aiConfigPage"));
    setProperty("cardStackPage", true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space16);

    auto *formPanel = new QFrame(this);
    formPanel->setObjectName(QStringLiteral("aiConfigSection"));
    formPanel->setAttribute(Qt::WA_StyledBackground, true);
    formPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    formPanel->setMinimumHeight(0);
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
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        return label;
    };
    auto makeHintLabel = [](const QString &text, QWidget *parent) {
        auto *label = new QLabel(text, parent);
        label->setObjectName(QStringLiteral("formFieldHint"));
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
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

    auto *connectionRow = new QWidget(formPanel);
    auto *connectionRowLayout = new QHBoxLayout(connectionRow);
    connectionRowLayout->setContentsMargins(0, 0, 0, 0);
    connectionRowLayout->setSpacing(PageLayout::Space8);
    m_connectionCombo = new QComboBox(connectionRow);
    m_connectionCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    PageLayout::configureFormInput(m_connectionCombo);
    auto *managerButton = new QPushButton(QStringLiteral("管理连接"), connectionRow);
    managerButton->setObjectName(QStringLiteral("secondaryButton"));
    managerButton->setFixedHeight(40);
    connect(managerButton, &QPushButton::clicked, this, &AiConfigToolPage::openConnectionManager);
    connectionRowLayout->addWidget(m_connectionCombo, 1);
    connectionRowLayout->addWidget(managerButton);
    formPanelLayout->addWidget(makeFormRow(QStringLiteral("AI 连接"),
                                           connectionRow,
                                           makeHintLabel(QStringLiteral("选择已保存的连接，自动填充 Base URL 和模型；可手动修改当前连接。"), formPanel),
                                           formPanel));

    auto *apiBaseUrlInput = new QLineEdit(formPanel);
    apiBaseUrlInput->setPlaceholderText(QStringLiteral("https://api.openai.com/v1"));
    PageLayout::configureFormInput(apiBaseUrlInput);
    m_apiBaseUrlInput = apiBaseUrlInput;
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
    m_apiKeyInput = apiKeyInput;
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
    modelInput->setInsertPolicy(QComboBox::NoInsert);
    modelInput->addItems(AiSettingsStore::defaultModelPresets());
    modelInput->setPlaceholderText(QStringLiteral("gpt-4o-mini"));
    modelInput->setFocusPolicy(Qt::StrongFocus);
    modelInput->setProperty("manualEdit", true);
    PageLayout::configureFormInput(modelInput);
    if (QLineEdit *le = modelInput->lineEdit()) {
        le->setReadOnly(false);
        le->setPlaceholderText(QStringLiteral("gpt-4o-mini"));
    }
    m_modelCombo = modelInput;
    formPanelLayout->addWidget(makeFormRow(QStringLiteral("模型"),
                                           modelInput,
                                           makeHintLabel(QStringLiteral("可手动输入任意模型名称，或从下拉选择。"), formPanel),
                                           formPanel));

    auto *rememberKeyCheck = new QCheckBox(QStringLiteral("记住 Key"), formPanel);
    rememberKeyCheck->setChecked(true);
    m_rememberKeyCheck = rememberKeyCheck;
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
    message->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_message = message;
    formPanelLayout->addWidget(message);
    layout->addWidget(formPanel, 1);

    auto *testClient = new OpenAiChatClient(this);

    auto reloadConnections = [this, modelInput](const QString &preserveModel = QString()) {
        if (m_aiSettings == nullptr) {
            return;
        }
        AiSettings settings;
        QString error;
        m_aiSettings->load(&settings, &error);
        m_connectionCombo->blockSignals(true);
        m_connectionCombo->clear();
        for (int i = 0; i < settings.connections.size(); ++i) {
            const AiConnection &connection = settings.connections.at(i);
            m_connectionCombo->addItem(connection.name, connection.id);
        }
        int activeIndex = -1;
        if (!settings.activeConnectionId.isEmpty()) {
            for (int i = 0; i < settings.connections.size(); ++i) {
                if (settings.connections.at(i).id == settings.activeConnectionId) {
                    activeIndex = i;
                    break;
                }
            }
        }
        if (activeIndex >= 0) {
            m_connectionCombo->setCurrentIndex(activeIndex);
        } else if (m_connectionCombo->count() > 0) {
            m_connectionCombo->setCurrentIndex(0);
        }
        m_connectionCombo->blockSignals(false);
        const QString modelToRestore = preserveModel.isEmpty()
            ? settings.connections.value(activeIndex).model
            : preserveModel;
        const QStringList models = collectModelChoices(settings.connections, modelToRestore);
        m_modelCombo->blockSignals(true);
        m_modelCombo->clear();
        m_modelCombo->addItems(models);
        m_modelCombo->setEditable(true);
        m_modelCombo->setInsertPolicy(QComboBox::NoInsert);
        m_modelCombo->setProperty("manualEdit", true);
        m_modelCombo->setFocusPolicy(Qt::StrongFocus);
        if (QLineEdit *le = m_modelCombo->lineEdit()) {
            le->setReadOnly(false);
        }
        if (!modelToRestore.isEmpty()) {
            const int modelIndex = m_modelCombo->findText(modelToRestore);
            if (modelIndex >= 0) {
                m_modelCombo->setCurrentIndex(modelIndex);
            } else {
                m_modelCombo->setEditText(modelToRestore);
            }
        }
        m_modelCombo->blockSignals(false);
    };

    connect(m_connectionCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this, reloadConnections](int index) {
                if (m_aiSettings == nullptr || index < 0) {
                    return;
                }
                AiSettings settings;
                QString error;
                m_aiSettings->load(&settings, &error);
                if (index >= settings.connections.size()) {
                    return;
                }
                const AiConnection &connection = settings.connections.at(index);
                settings.activeConnectionId = connection.id;
                QString saveError;
                m_aiSettings->save(settings, &saveError);
                m_apiBaseUrlInput->setText(connection.apiBaseUrl);
                m_modelCombo->blockSignals(true);
                {
                    const QStringList models = collectModelChoices(settings.connections,
                                                                   connection.model);
                    m_modelCombo->clear();
                    m_modelCombo->addItems(models);
                    m_modelCombo->setEditable(true);
                    m_modelCombo->setInsertPolicy(QComboBox::NoInsert);
                    m_modelCombo->setProperty("manualEdit", true);
                    m_modelCombo->setFocusPolicy(Qt::StrongFocus);
                    if (QLineEdit *le = m_modelCombo->lineEdit()) {
                        le->setReadOnly(false);
                    }
                    const int modelIndex = m_modelCombo->findText(connection.model);
                    if (modelIndex >= 0) {
                        m_modelCombo->setCurrentIndex(modelIndex);
                    } else {
                        m_modelCombo->setEditText(connection.model);
                    }
                }
                m_modelCombo->blockSignals(false);
                m_apiKeyInput->clear();
                m_apiKeyInput->setPlaceholderText(QStringLiteral("••••••••••••••••"));
                if (m_credentials != nullptr && m_credentials->has(connection.credentialRef)) {
                    m_apiKeyInput->setText(m_credentials->load(connection.credentialRef));
                }
                m_message->clear();
                reloadConnections();
            });

    connect(saveButton, &QPushButton::clicked, this, [this, reloadConnections]() {
        if (m_aiSettings == nullptr) {
            m_message->setText(QStringLiteral("AI 配置存储未初始化"));
            return;
        }
        AiSettings settings;
        QString loadError;
        if (!m_aiSettings->load(&settings, &loadError)) {
            m_message->setText(loadError);
            return;
        }
        int connectionIndex = m_connectionCombo->currentIndex();
        if (connectionIndex < 0 || connectionIndex >= settings.connections.size()) {
            AiConnection created;
            created.id = AiSettings::generateConnectionId();
            created.name = QStringLiteral("新连接");
            created.credentialRef = AiSettingsStore::buildCredentialRef(created.id);
            settings.connections.append(created);
            connectionIndex = settings.connections.size() - 1;
            settings.activeConnectionId = created.id;
        }
        AiConnection &target = settings.connections[connectionIndex];
        if (target.name.isEmpty()) {
            target.name = QStringLiteral("未命名连接");
        }
        if (target.credentialRef.isEmpty()) {
            target.credentialRef = AiSettingsStore::buildCredentialRef(target.id);
        }
        target.apiBaseUrl = m_apiBaseUrlInput->text().trimmed();
        m_modelCombo->lineEdit()->editingFinished();
        target.model = m_modelCombo->currentText().trimmed();
        settings.apiBaseUrl = target.apiBaseUrl;
        settings.model = target.model;
        settings.credentialRef = target.credentialRef;
        settings.rememberKey = m_rememberKeyCheck->isChecked();
        settings.activeConnectionId = target.id;

        if (target.apiBaseUrl.isEmpty()) {
            m_message->setText(QStringLiteral("请填写 API Base URL"));
            return;
        }
        if (target.model.isEmpty()) {
            m_message->setText(QStringLiteral("请填写模型名称"));
            return;
        }

        QString saveError;
        if (!m_aiSettings->save(settings, &saveError)) {
            m_message->setText(saveError);
            return;
        }

        if (settings.rememberKey) {
            if (!m_apiKeyInput->text().isEmpty()) {
                if (m_credentials == nullptr) {
                    m_message->setText(QStringLiteral("凭据存储未初始化"));
                    return;
                }
                if (!m_credentials->save(target.credentialRef, m_apiKeyInput->text(), &saveError)) {
                    m_message->setText(saveError);
                    return;
                }
            } else if (m_credentials == nullptr || !m_credentials->has(target.credentialRef)) {
                m_message->setText(QStringLiteral("勾选记住 Key 时请填写 API Key"));
                return;
            }
        } else if (m_credentials != nullptr) {
            m_credentials->remove(target.credentialRef, &saveError);
        }

        m_message->setText(QStringLiteral("配置已保存"));
        loadForm();
    });

    connect(testButton, &QPushButton::clicked, this, [this, testClient, saveButton, testButton]() {
        const QString baseUrl = m_apiBaseUrlInput->text().trimmed();
        m_modelCombo->lineEdit()->editingFinished();
        const QString model = m_modelCombo->currentText().trimmed();
        QString key = m_apiKeyInput->text();
        if (key.isEmpty() && m_credentials != nullptr && m_aiSettings != nullptr) {
            AiSettings stored;
            m_aiSettings->load(&stored, nullptr);
            const int idx = findConnectionIndex(stored.activeConnectionId);
            if (idx >= 0) {
                const AiConnection &connection = stored.connections.at(idx);
                if (m_credentials->has(connection.credentialRef)) {
                    key = m_credentials->load(connection.credentialRef);
                }
            }
        }
        if (baseUrl.isEmpty() || model.isEmpty() || key.isEmpty()) {
            m_message->setText(QStringLiteral("请先填写 URL、模型和 API Key"));
            return;
        }

        testClient->abort();
        saveButton->setEnabled(false);
        testButton->setEnabled(false);
        m_message->setText(QStringLiteral("正在测试连接..."));

        connect(testClient, &OpenAiChatClient::finished, this, [this, testButton, saveButton]() {
            m_message->setText(QStringLiteral("连接成功"));
            testButton->setEnabled(true);
            saveButton->setEnabled(true);
        }, Qt::SingleShotConnection);
        connect(testClient, &OpenAiChatClient::failed, this, [this, testButton, saveButton](const QString &error) {
            m_message->setText(QStringLiteral("连接失败：%1").arg(error));
            testButton->setEnabled(true);
            saveButton->setEnabled(true);
        }, Qt::SingleShotConnection);

        testClient->testConnection(baseUrl, key, model);
    });
}

int AiConfigToolPage::findConnectionIndex(const QString &id) const
{
    if (m_aiSettings == nullptr) {
        return -1;
    }
    AiSettings settings;
    m_aiSettings->load(&settings, nullptr);
    for (int i = 0; i < settings.connections.size(); ++i) {
        if (settings.connections.at(i).id == id) {
            return i;
        }
    }
    return -1;
}

void AiConfigToolPage::loadForm()
{
    if (m_aiSettings == nullptr) {
        return;
    }
    AiSettings settings;
    QString error;
    if (!m_aiSettings->load(&settings, &error)) {
        m_message->setText(error);
        return;
    }
    m_connectionCombo->blockSignals(true);
    m_connectionCombo->clear();
    for (const AiConnection &connection : settings.connections) {
        m_connectionCombo->addItem(connection.name, connection.id);
    }
    int activeIndex = -1;
    for (int i = 0; i < settings.connections.size(); ++i) {
        if (settings.connections.at(i).id == settings.activeConnectionId) {
            activeIndex = i;
            break;
        }
    }
    if (activeIndex < 0 && m_connectionCombo->count() > 0) {
        activeIndex = 0;
    }
    m_connectionCombo->setCurrentIndex(activeIndex);
    m_connectionCombo->blockSignals(false);

    const AiConnection &current = settings.connections.value(activeIndex);
    m_apiBaseUrlInput->setText(current.apiBaseUrl);
    m_modelCombo->blockSignals(true);
    m_modelCombo->clear();
    m_modelCombo->addItems(collectModelChoices(settings.connections, current.model));
    m_modelCombo->setEditable(true);
    m_modelCombo->setInsertPolicy(QComboBox::NoInsert);
    m_modelCombo->setProperty("manualEdit", true);
    m_modelCombo->setFocusPolicy(Qt::StrongFocus);
    if (QLineEdit *le = m_modelCombo->lineEdit()) {
        le->setReadOnly(false);
    }
    {
        const int idx = m_modelCombo->findText(current.model);
        if (idx >= 0) {
            m_modelCombo->setCurrentIndex(idx);
        } else {
            m_modelCombo->setEditText(current.model);
        }
    }
    m_modelCombo->blockSignals(false);
    m_rememberKeyCheck->setChecked(settings.rememberKey);
    m_apiKeyInput->clear();
    m_apiKeyInput->setPlaceholderText(QStringLiteral("••••••••••••••••"));
    if (m_credentials != nullptr && m_credentials->has(current.credentialRef)) {
        m_apiKeyInput->setText(m_credentials->load(current.credentialRef));
    }
    m_message->clear();
}

void AiConfigToolPage::openConnectionManager()
{
    AiConnectionManagerDialog dialog(m_credentials, this);
    if (dialog.exec() == QDialog::Accepted) {
        loadForm();
    }
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
