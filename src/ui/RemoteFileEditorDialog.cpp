#include "ui/RemoteFileEditorDialog.h"

#include "infra/AppBranding.h"
#include "ui/PageLayout.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

RemoteFileEditorDialog::RemoteFileEditorDialog(const QString &remotePath,
                                               const QString &content,
                                               bool readOnly,
                                               const SaveHandler &saveHandler,
                                               QWidget *parent)
    : QDialog(parent)
    , m_remotePath(remotePath)
    , m_saveHandler(saveHandler)
{
    setWindowTitle(readOnly
                       ? QStringLiteral("查看远程文件 - %1").arg(remotePath)
                       : QStringLiteral("编辑远程文件 - %1").arg(remotePath));
    setModal(true);
    PageLayout::applyRemoteToolDialog(this);
    AppBranding::applyWindowIcon(this);

    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *pathLabel = new QLabel(remotePath);
    pathLabel->setWordWrap(true);
    layout->addWidget(pathLabel);

    m_editor = new QPlainTextEdit;
    m_editor->setPlainText(content);
    m_editor->setReadOnly(readOnly);
    m_editor->setMinimumHeight(360);
    layout->addWidget(m_editor, 1);

    auto *buttons = new QDialogButtonBox;
    if (!readOnly) {
        auto *saveButton = buttons->addButton(QStringLiteral("保存"), QDialogButtonBox::AcceptRole);
        saveButton->setObjectName(QStringLiteral("primaryButton"));
        connect(saveButton, &QPushButton::clicked, this, &RemoteFileEditorDialog::onSave);
    }
    auto *closeButton = buttons->addButton(QDialogButtonBox::Close);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void RemoteFileEditorDialog::onSave()
{
    if (!m_saveHandler) {
        reject();
        return;
    }

    QString error;
    if (!m_saveHandler(m_editor->toPlainText(), &error)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), error);
        return;
    }
    accept();
}
