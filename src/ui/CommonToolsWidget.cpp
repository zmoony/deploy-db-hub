#include "ui/CommonToolsWidget.h"

#include "ui/AiChatWidget.h"
#include "adapters/ai/OpenAiChatClient.h"
#include "ui/AiAssistHelper.h"
#include "ui/tools/pages/AiConfigToolPage.h"
#include "ui/tools/pages/Base64TextToolPage.h"
#include "ui/tools/pages/CaseToolPage.h"
#include "ui/tools/pages/CronToolPage.h"
#include "ui/tools/pages/DiffToolPage.h"
#include "ui/tools/pages/HashToolPage.h"
#include "ui/tools/pages/HtmlEntityToolPage.h"
#include "ui/tools/pages/HttpStatusToolPage.h"
#include "ui/tools/pages/JsonToolPage.h"
#include "ui/tools/pages/NumberBaseToolPage.h"
#include "ui/tools/pages/RegexToolPage.h"
#include "ui/tools/pages/TimestampToolPage.h"
#include "ui/tools/pages/UrlCodecToolPage.h"
#include "ui/tools/pages/UuidToolPage.h"
#include "ui/AiStreamBuffer.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "tools/CommonTools.h"
#include "ui/HttpRequestWidget.h"
#include "ui/PageLayout.h"
#include "ui/WebSocketToolWidget.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QBuffer>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPair>
#include <QPainterPath>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QSharedPointer>
#include <QSpinBox>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QPushButton *makeActionButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("toolBarButton"));
    button->setMinimumHeight(28);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

QPlainTextEdit *makeEditor(const QString &placeholder, QWidget *parent)
{
    auto *editor = new QPlainTextEdit(parent);
    editor->setPlaceholderText(placeholder);
    editor->setMinimumHeight(220);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    return editor;
}

void copyTextToClipboard(const QString &text)
{
    if (QClipboard *clipboard = QApplication::clipboard()) {
        clipboard->setText(text);
    }
}

}

CommonToolsWidget::CommonToolsWidget(AiSettingsStore *aiSettings,
                                     CredentialStore *credentials,
                                     QWidget *parent)
    : QWidget(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(new Ui::Tools::AiConfigToolPage(m_aiSettings, m_credentials, this));
    m_stack->addWidget(new AiChatWidget(m_aiSettings, m_credentials, this));
    m_stack->addWidget(new Ui::Tools::JsonToolPage(this));
    m_stack->addWidget(new Ui::Tools::DiffToolPage(this));
    m_stack->addWidget(buildImageBase64Page());
    m_stack->addWidget(new Ui::Tools::RegexToolPage(this));
    m_stack->addWidget(new Ui::Tools::CronToolPage(this));
    m_stack->addWidget(new Ui::Tools::TimestampToolPage(this));
    m_stack->addWidget(new Ui::Tools::HtmlEntityToolPage(this));
    m_stack->addWidget(new Ui::Tools::HttpStatusToolPage(this));
    m_stack->addWidget(buildTextToolPage(
        QStringLiteral("Hex 转字符串"),
        QStringLiteral("将十六进制字节转为 UTF-8 字符串。"),
        QStringLiteral("转换"),
        QString(),
        QStringLiteral("48656c6c6f205174")));
    m_stack->addWidget(buildWebSocketPage());
    m_stack->addWidget(buildHttpRequestPage());
    m_stack->addWidget(buildTextToolPage(
        QStringLiteral("契约 Mock 数据"),
        QStringLiteral("粘贴 Swagger/Thrift 定义或 JSON 示例，生成多组 mock 数据。"),
        QStringLiteral("生成 Mock"),
        QString(),
        QStringLiteral("{\"name\":\"demo\",\"count\":1,\"enabled\":true}"),
        true,
        QStringLiteral("You generate mock JSON data. Reply with a JSON array only, no markdown or explanation.")));
    m_stack->addWidget(buildTextToolPage(
        QStringLiteral("数据采样脱敏"),
        QStringLiteral("从样本数据中抽样并按规则脱敏导出。"),
        QStringLiteral("脱敏"),
        QString(),
        QStringLiteral("phone=13812345678 email=a@example.com")));
    m_stack->addWidget(new Ui::Tools::UuidToolPage(this));
    m_stack->addWidget(new Ui::Tools::HashToolPage(this));
    m_stack->addWidget(new Ui::Tools::UrlCodecToolPage(this));
    m_stack->addWidget(new Ui::Tools::Base64TextToolPage(this));
    m_stack->addWidget(buildJwtPage());
    m_stack->addWidget(new Ui::Tools::NumberBaseToolPage(this));
    m_stack->addWidget(new Ui::Tools::CaseToolPage(this));

    layout->addWidget(m_stack, 1);
}

QStringList CommonToolsWidget::toolLabels() const
{
    return {
        QStringLiteral("AI 配置"),
        QStringLiteral("AI 聊天"),
        QStringLiteral("JSON 格式化"),
        QStringLiteral("文本比较"),
        QStringLiteral("图片/Base64"),
        QStringLiteral("正则表达式"),
        QStringLiteral("Cron 表达式"),
        QStringLiteral("时间戳"),
        QStringLiteral("HTML 特殊字符"),
        QStringLiteral("HTTP 状态码"),
        QStringLiteral("Hex 转字符串"),
        QStringLiteral("WebSocket 测试"),
        QStringLiteral("HTTP 请求调试"),
        QStringLiteral("契约 Mock 数据"),
        QStringLiteral("数据采样脱敏"),
        QStringLiteral("UUID 生成"),
        QStringLiteral("哈希计算"),
        QStringLiteral("URL 编解码"),
        QStringLiteral("Base64 文本"),
        QStringLiteral("JWT 解析"),
        QStringLiteral("进制转换"),
        QStringLiteral("命名转换")
    };
}

QWidget *CommonToolsWidget::takeToolPage(int index)
{
    if (m_stack == nullptr || index < 0 || index >= m_stack->count()) {
        return nullptr;
    }
    QWidget *page = m_stack->widget(index);
    m_stack->removeWidget(page);
    page->setParent(nullptr);
    return page;
}

int CommonToolsWidget::toolPageCount() const
{
    return m_stack != nullptr ? m_stack->count() : 0;
}

QWidget *CommonToolsWidget::buildTextToolPage(const QString &title,
                                              const QString &subtitle,
                                              const QString &primaryAction,
                                              const QString &secondaryAction,
                                              const QString &placeholder,
                                              bool enableAiAssist,
                                              const QString &aiSystemPrompt)
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *primaryButton = makeActionButton(primaryAction, toolbar);
    toolbarLayout->addWidget(primaryButton);
    QPushButton *secondaryButton = nullptr;
    if (!secondaryAction.isEmpty()) {
        secondaryButton = makeActionButton(secondaryAction, toolbar);
        toolbarLayout->addWidget(secondaryButton);
    }
    QPushButton *aiAssistButton = nullptr;
    QPushButton *aiStopButton = nullptr;
    if (enableAiAssist) {
        aiAssistButton = makeActionButton(QStringLiteral("AI 辅助"), toolbar);
        aiStopButton = makeActionButton(QStringLiteral("停止"), toolbar);
        aiStopButton->setEnabled(false);
        toolbarLayout->addWidget(aiAssistButton);
        toolbarLayout->addWidget(aiStopButton);
    }
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(page);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = makeEditor(placeholder, editors);
    auto *output = makeEditor(QStringLiteral("输出"), editors);
    output->setReadOnly(true);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(primaryButton, &QPushButton::clicked, this, [this, title, input, output, message]() {
        QString error;
        QString result;
        if (title == QStringLiteral("JSON 格式化")) {
            result = CommonTools::formatJson(input->toPlainText(), &error);
        } else if (title == QStringLiteral("文件差异")) {
            const QStringList parts = input->toPlainText().split(QStringLiteral("\n---\n"));
            if (parts.size() != 2) {
                error = QStringLiteral("请用单独一行 --- 分隔两段文本");
            } else {
                result = CommonTools::compareLines(parts.at(0), parts.at(1));
            }
        } else if (title == QStringLiteral("图片/Base64")) {
            result = CommonTools::textToBase64(input->toPlainText());
        } else if (title == QStringLiteral("正则表达式")) {
            const QStringList parts = input->toPlainText().split(QStringLiteral("\n---\n"));
            if (parts.size() != 2) {
                error = QStringLiteral("请用单独一行 --- 分隔正则和样本文本");
            } else {
                result = CommonTools::matchRegularExpression(parts.at(0), parts.at(1), &error);
            }
        } else if (title == QStringLiteral("Cron 表达式")) {
            result = CommonTools::describeCron(input->toPlainText(), &error);
        } else if (title == QStringLiteral("时间戳")) {
            result = CommonTools::timestampToLocalText(input->toPlainText(), &error);
        } else if (title == QStringLiteral("Hex 转字符串")) {
            result = CommonTools::hexToString(input->toPlainText(), &error);
        } else if (title == QStringLiteral("契约 Mock 数据")) {
            result = CommonTools::mockFromJsonExample(input->toPlainText(), &error);
        } else if (title == QStringLiteral("数据采样脱敏")) {
            result = CommonTools::maskSensitiveText(input->toPlainText());
        }
        setOutput(output, message, result, error);
    });

    if (secondaryButton != nullptr) {
        connect(secondaryButton, &QPushButton::clicked, this, [this, input, output, message]() {
            QString error;
            const QString result = CommonTools::base64ToText(input->toPlainText(), &error);
            setOutput(output, message, result, error);
        });
    }

    if (enableAiAssist && aiAssistButton != nullptr && aiStopButton != nullptr) {
        wireAiAssist(page,
                     aiAssistButton,
                     aiStopButton,
                     output,
                     message,
                     [input]() { return input->toPlainText(); },
                     aiSystemPrompt);
    }

    return page;
}

QWidget *CommonToolsWidget::buildImageBase64Page()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
struct ImageState {
        QByteArray bytes;
        QString mime;
    };
    auto state = QSharedPointer<ImageState>::create();

    auto *urlRow = new QWidget(page);
    auto *urlLayout = new QHBoxLayout(urlRow);
    urlLayout->setContentsMargins(0, 0, 0, 0);
    urlLayout->setSpacing(PageLayout::Space8);
    auto *pickButton = makeActionButton(QStringLiteral("选择本地图片"), urlRow);
    auto *urlEdit = new QLineEdit(urlRow);
    urlEdit->setPlaceholderText(QStringLiteral("https://example.com/image.png"));
    PageLayout::configureFormInput(urlEdit);
    auto *urlButton = makeActionButton(QStringLiteral("从 URL 加载"), urlRow);
    urlLayout->addWidget(pickButton);
    urlLayout->addWidget(urlEdit, 1);
    urlLayout->addWidget(urlButton);
    layout->addWidget(urlRow);

    auto *actions = new QWidget(page);
    auto *actionsLayout = new QHBoxLayout(actions);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(PageLayout::Space8);
    auto *toBase64 = makeActionButton(QStringLiteral("图片 → Base64"), actions);
    auto *toImage = makeActionButton(QStringLiteral("Base64 → 图片"), actions);
    auto *copyButton = makeActionButton(QStringLiteral("复制 Base64"), actions);
    auto *clearButton = makeActionButton(QStringLiteral("清空"), actions);
    actionsLayout->addWidget(toBase64);
    actionsLayout->addWidget(toImage);
    actionsLayout->addWidget(copyButton);
    actionsLayout->addWidget(clearButton);
    actionsLayout->addStretch();
    layout->addWidget(actions);

    auto *body = new QWidget(page);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(PageLayout::Space12);

    auto *base64Edit = new QPlainTextEdit(body);
    base64Edit->setPlaceholderText(QStringLiteral("Base64 文本（可含 data:image/...;base64, 前缀）"));
    base64Edit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    base64Edit->setMinimumHeight(260);

    auto *previewArea = new QScrollArea(body);
    previewArea->setWidgetResizable(true);
    auto *preview = new QLabel(previewArea);
    preview->setAlignment(Qt::AlignCenter);
    preview->setText(QStringLiteral("图片预览"));
    previewArea->setWidget(preview);

    bodyLayout->addWidget(base64Edit, 1);
    bodyLayout->addWidget(previewArea, 1);
    layout->addWidget(body, 1);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    auto mimeForPath = [](const QString &path) -> QString {
        const QString lower = path.toLower();
        if (lower.endsWith(QStringLiteral(".jpg")) || lower.endsWith(QStringLiteral(".jpeg"))) {
            return QStringLiteral("image/jpeg");
        }
        if (lower.endsWith(QStringLiteral(".gif"))) {
            return QStringLiteral("image/gif");
        }
        if (lower.endsWith(QStringLiteral(".bmp"))) {
            return QStringLiteral("image/bmp");
        }
        if (lower.endsWith(QStringLiteral(".webp"))) {
            return QStringLiteral("image/webp");
        }
        return QStringLiteral("image/png");
    };

    auto showImage = [preview, message](const QByteArray &bytes) -> bool {
        QImage image;
        if (!image.loadFromData(bytes)) {
            message->setText(QStringLiteral("无法识别图片数据"));
            return false;
        }
        QPixmap pixmap = QPixmap::fromImage(image);
        if (pixmap.width() > 520 || pixmap.height() > 360) {
            pixmap = pixmap.scaled(520, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        preview->setPixmap(pixmap);
        return true;
    };

    connect(pickButton, &QPushButton::clicked, this,
            [this, state, showImage, mimeForPath, message](){
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择图片"), QString(),
            QStringLiteral("图片 (*.png *.jpg *.jpeg *.gif *.bmp *.webp)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            message->setText(QStringLiteral("无法读取文件"));
            return;
        }
        const QByteArray bytes = file.readAll();
        if (showImage(bytes)) {
            state->bytes = bytes;
            state->mime = mimeForPath(path);
            message->setText(QStringLiteral("已加载本地图片（%1 字节）").arg(bytes.size()));
        }
    });

    connect(urlButton, &QPushButton::clicked, this,
            [this, state, showImage, urlEdit, message](){
        const QUrl url(urlEdit->text().trimmed());
        if (!url.isValid() || url.scheme().isEmpty()) {
            message->setText(QStringLiteral("URL 无效"));
            return;
        }
        auto *manager = new QNetworkAccessManager(this);
        QNetworkRequest request(url);
        QNetworkReply *reply = manager->get(request);
        message->setText(QStringLiteral("正在下载图片..."));
        QObject::connect(reply, &QNetworkReply::finished, this,
                         [reply, manager, state, showImage, message]() {
            if (reply->error() != QNetworkReply::NoError) {
                message->setText(QStringLiteral("下载失败：%1").arg(reply->errorString()));
            } else {
                const QByteArray bytes = reply->readAll();
                const QString contentType =
                    reply->header(QNetworkRequest::ContentTypeHeader).toString();
                if (showImage(bytes)) {
                    state->bytes = bytes;
                    state->mime = contentType.isEmpty() ? QStringLiteral("image/png")
                                                        : contentType.section(QLatin1Char(';'), 0, 0);
                    message->setText(QStringLiteral("已加载网络图片（%1 字节）").arg(bytes.size()));
                }
            }
            reply->deleteLater();
            manager->deleteLater();
        });
    });

    connect(toBase64, &QPushButton::clicked, page, [state, base64Edit, message]() {
        if (state->bytes.isEmpty()) {
            message->setText(QStringLiteral("请先加载图片"));
            return;
        }
        const QString mime = state->mime.isEmpty() ? QStringLiteral("image/png") : state->mime;
        const QString encoded = QStringLiteral("data:%1;base64,%2")
            .arg(mime, QString::fromLatin1(state->bytes.toBase64()));
        base64Edit->setPlainText(encoded);
        message->setText(QStringLiteral("已转换为 Base64"));
    });

    connect(toImage, &QPushButton::clicked, page, [state, base64Edit, showImage, message]() {
        QString text = base64Edit->toPlainText().trimmed();
        const int comma = text.indexOf(QLatin1Char(','));
        if (text.startsWith(QStringLiteral("data:")) && comma >= 0) {
            text = text.mid(comma + 1);
        }
        text.remove(QLatin1Char('\n'));
        text.remove(QLatin1Char('\r'));
        text.remove(QLatin1Char(' '));
        const QByteArray bytes = QByteArray::fromBase64(text.toLatin1());
        if (bytes.isEmpty()) {
            message->setText(QStringLiteral("Base64 内容为空或无效"));
            return;
        }
        if (showImage(bytes)) {
            state->bytes = bytes;
            message->setText(QStringLiteral("已还原为图片"));
        }
    });

    connect(copyButton, &QPushButton::clicked, page, [base64Edit]() {
        copyTextToClipboard(base64Edit->toPlainText());
    });
    connect(clearButton, &QPushButton::clicked, page, [state, base64Edit, preview, message]() {
        state->bytes.clear();
        state->mime.clear();
        base64Edit->clear();
        preview->setText(QStringLiteral("图片预览"));
        preview->setPixmap(QPixmap());
        message->clear();
    });

    return page;
}

QWidget *CommonToolsWidget::buildJwtPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *parse = makeActionButton(QStringLiteral("解析"), toolbar);
    auto *copy = makeActionButton(QStringLiteral("复制结果"), toolbar);
    toolbarLayout->addWidget(parse);
    toolbarLayout->addWidget(copy);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *input = makeEditor(QStringLiteral("粘贴 JWT (xxxxx.yyyyy.zzzzz)"), page);
    input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    auto *output = makeEditor(QStringLiteral("解析结果"), page);
    output->setReadOnly(true);
    layout->addWidget(input, 1);
    layout->addWidget(output, 1);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(parse, &QPushButton::clicked, page, [input, output, message]() {
        QString error;
        const QString result = CommonTools::decodeJwt(input->toPlainText(), &error);
        if (!error.isEmpty()) {
            output->clear();
            message->setText(error);
            return;
        }
        output->setPlainText(result);
        message->setText(QStringLiteral("已解析"));
    });
    connect(copy, &QPushButton::clicked, page, [output]() {
        copyTextToClipboard(output->toPlainText());
    });

    return page;
}

QWidget *CommonToolsWidget::buildHttpRequestPage()
{
    return new HttpRequestWidget(this);
}

QWidget *CommonToolsWidget::buildWebSocketPage()
{
    return new WebSocketToolWidget(this);
}

void CommonToolsWidget::setOutput(QPlainTextEdit *output, QLabel *message, const QString &text, const QString &error)
{
    if (!error.isEmpty()) {
        output->clear();
        message->setText(error);
        return;
    }
    output->setPlainText(text);
    message->setText(QStringLiteral("已完成"));
}

bool CommonToolsWidget::resolveAiCredentials(AiSettings *settings, QString *apiKey, QString *error) const
{
    return AiAssistHelper::resolveCredentials(m_aiSettings, m_credentials, settings, apiKey, error);
}

void CommonToolsWidget::wireAiAssist(QWidget *page,
                                     QPushButton *assistButton,
                                     QPushButton *stopButton,
                                     QPlainTextEdit *output,
                                     QLabel *message,
                                     const std::function<QString()> &userContentProvider,
                                     const QString &systemPrompt,
                                     const std::function<void()> &onStart)
{
    auto *client = new OpenAiChatClient(page);
    auto *buffer = new AiStreamBuffer(output, page);

    auto setBusy = [assistButton, stopButton](bool busy) {
        assistButton->setEnabled(!busy);
        stopButton->setEnabled(busy);
    };

    connect(client, &OpenAiChatClient::deltaReceived, page, [buffer](const QString &chunk) {
        buffer->append(chunk);
    });
    connect(client, &OpenAiChatClient::finished, page, [message, buffer, setBusy]() {
        buffer->flush();
        message->setText(QStringLiteral("AI 已完成"));
        setBusy(false);
    });
    connect(client, &OpenAiChatClient::failed, page, [message, buffer, setBusy](const QString &failedMessage) {
        buffer->flush();
        message->setText(QStringLiteral("AI 失败：%1").arg(failedMessage));
        setBusy(false);
    });

    connect(assistButton, &QPushButton::clicked, page, [this, client, output, message, buffer, userContentProvider, systemPrompt, onStart, setBusy]() {
        const QString userContent = userContentProvider().trimmed();
        if (userContent.isEmpty()) {
            message->setText(QStringLiteral("请先输入内容"));
            return;
        }

        AiSettings settings;
        QString apiKey;
        QString error;
        if (!resolveAiCredentials(&settings, &apiKey, &error)) {
            message->setText(error);
            return;
        }

        if (onStart) {
            onStart();
        }

        client->abort();
        buffer->reset();
        output->clear();
        setBusy(true);
        message->setText(QStringLiteral("AI 生成中..."));

        const QJsonArray messages{
            QJsonObject{
                {QStringLiteral("role"), QStringLiteral("system")},
                {QStringLiteral("content"), systemPrompt}
            },
            QJsonObject{
                {QStringLiteral("role"), QStringLiteral("user")},
                {QStringLiteral("content"), userContent}
            }
        };
        client->streamChat(settings.apiBaseUrl, apiKey, settings.model, messages);
    });

    connect(stopButton, &QPushButton::clicked, page, [client, message, buffer, setBusy]() {
        client->abort();
        buffer->flush();
        message->setText(QStringLiteral("已停止"));
        setBusy(false);
    });
}
