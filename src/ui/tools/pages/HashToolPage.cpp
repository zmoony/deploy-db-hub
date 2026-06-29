#include "ui/tools/pages/HashToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"
#include "ui/tools/ToolEditor.h"
#include "ui/tools/ToolResultRow.h"

#include "ui/tools/ToolUiHelpers.h"
#include <QPushButton>
#include <QVBoxLayout>


namespace Ui {
namespace Tools {

HashToolPage::HashToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("hashToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *input = new ToolEditor(this);
    input->setPlaceholderText(QStringLiteral("待计算文本"));
    layout->addWidget(input, 1);

    auto *compute = Helpers::makeToolButton(QStringLiteral("计算哈希"), this);
    layout->addWidget(compute, 0, Qt::AlignLeft);

    auto *md5 = new ToolResultRow(QStringLiteral("MD5"), this);
    auto *sha1 = new ToolResultRow(QStringLiteral("SHA1"), this);
    auto *sha256 = new ToolResultRow(QStringLiteral("SHA256"), this);
    auto *sha512 = new ToolResultRow(QStringLiteral("SHA512"), this);

    auto *rowsLayout = new QVBoxLayout;
    rowsLayout->setContentsMargins(0, 0, 0, 0);
    rowsLayout->setSpacing(PageLayout::Space8);
    rowsLayout->addWidget(md5);
    rowsLayout->addWidget(sha1);
    rowsLayout->addWidget(sha256);
    rowsLayout->addWidget(sha512);
    layout->addLayout(rowsLayout);
    layout->addStretch();

    connect(compute, &QPushButton::clicked, this, [input, md5, sha1, sha256, sha512]() {
        const CommonTools::HashResult result =
            CommonTools::computeHashes(input->text().toUtf8());
        md5->setText(result.md5);
        sha1->setText(result.sha1);
        sha256->setText(result.sha256);
        sha512->setText(result.sha512);
    });
}

QString HashToolPage::title() const
{
    return QStringLiteral("Hash 计算");
}

QString HashToolPage::subtitle() const
{
    return QStringLiteral("计算文本的 MD5/SHA1/SHA256");
}

} // namespace Tools
} // namespace Ui
