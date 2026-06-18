#include "ui/DeploymentLogDialog.h"

#include "infra/AppBranding.h"
#include "infra/LocalLogCatalog.h"
#include "ui/PageLayout.h"

#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

bool DeploymentLogDialog::canOpen(const QString &relativePath)
{
    return LocalLogCatalog::canOpen(relativePath);
}

QString DeploymentLogDialog::loadContent(const QString &relativePath, QString *error)
{
    return LocalLogCatalog::loadContent(relativePath, error);
}

DeploymentLogDialog::DeploymentLogDialog(const QString &relativePath, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("部署日志"));
    setModal(true);
    PageLayout::applyModalDialog(this);
    AppBranding::applyWindowIcon(this);

    QString error;
    const QString content = loadContent(relativePath, &error);
    buildUi(relativePath, content.isEmpty() ? error : content);
}

void DeploymentLogDialog::buildUi(const QString &relativePath, const QString &content)
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *editor = new QPlainTextEdit;
    editor->setReadOnly(true);
    editor->setPlainText(content);
    editor->setMinimumHeight(280);
    layout->addWidget(editor, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(buttons);

    setWindowTitle(QStringLiteral("部署日志 - %1").arg(relativePath));
}
