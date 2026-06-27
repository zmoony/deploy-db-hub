#include "ui/tools/pages/ImageBase64ToolPage.h"

#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSharedPointer>
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

void copyTextToClipboard(const QString &text)
{
    if (QClipboard *clipboard = QApplication::clipboard()) {
        clipboard->setText(text);
    }
}

} // namespace

namespace Ui {
namespace Tools {

ImageBase64ToolPage::ImageBase64ToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("imageBase64ToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    struct ImageState {
        QByteArray bytes;
        QString mime;
    };
    auto state = QSharedPointer<ImageState>::create();

    auto *urlRow = new QWidget(this);
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

    auto *actions = new QWidget(this);
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

    auto *body = new QWidget(this);
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

    auto *message = new QLabel(this);
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

    connect(toBase64, &QPushButton::clicked, this, [state, base64Edit, message]() {
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

    connect(toImage, &QPushButton::clicked, this, [state, base64Edit, showImage, message]() {
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

    connect(copyButton, &QPushButton::clicked, this, [base64Edit]() {
        copyTextToClipboard(base64Edit->toPlainText());
    });
    connect(clearButton, &QPushButton::clicked, this, [state, base64Edit, preview, message]() {
        state->bytes.clear();
        state->mime.clear();
        base64Edit->clear();
        preview->setText(QStringLiteral("图片预览"));
        preview->setPixmap(QPixmap());
        message->clear();
    });
}

QString ImageBase64ToolPage::title() const
{
    return QStringLiteral("图片 Base64");
}

QString ImageBase64ToolPage::subtitle() const
{
    return QStringLiteral("图片与 Base64 互转");
}

} // namespace Tools
} // namespace Ui
