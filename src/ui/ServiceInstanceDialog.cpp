#include "ui/ServiceInstanceDialog.h"

#include "ui/PageLayout.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

ServiceInstanceDialog::ServiceInstanceDialog(ServiceProductKind product, QWidget *parent)
    : QDialog(parent)
    , m_product(product)
{
    setWindowTitle(QStringLiteral("新建 %1 实例").arg(serviceProductKindLabel(product)));

    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    QFormLayout *form = nullptr;
    layout->addWidget(PageLayout::wrapDialogFormSection(QStringLiteral("实例信息"), this, &form));

    m_id = new QLineEdit;
    m_name = new QLineEdit;
    PageLayout::configureFormInput(m_id);
    PageLayout::configureFormInput(m_name);
    m_id->setPlaceholderText(QStringLiteral("例如 view-test-kafka"));
    m_name->setPlaceholderText(QStringLiteral("例如 视图库测试kafka"));
    form->addRow(QStringLiteral("实例 ID"), m_id);
    form->addRow(QStringLiteral("实例名称"), m_name);

    auto *footer = PageLayout::makeDialogFooter(this);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, PageLayout::Space12, 0, 0);
    footerLayout->setSpacing(PageLayout::Space8);
    footerLayout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, footer);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("primaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &ServiceInstanceDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    footerLayout->addWidget(buttons);
    layout->addWidget(footer);

    PageLayout::applyCompactModalDialog(this);
}

void ServiceInstanceDialog::setInstance(const QJsonObject &instance, bool editMode)
{
    m_editMode = editMode;
    m_id->setText(instance.value(QStringLiteral("id")).toString());
    m_name->setText(instance.value(QStringLiteral("name")).toString());
    m_id->setReadOnly(editMode);
    setWindowTitle(editMode
                        ? QStringLiteral("编辑 %1 实例").arg(serviceProductKindLabel(m_product))
                        : QStringLiteral("新建 %1 实例").arg(serviceProductKindLabel(m_product)));
}

QJsonObject ServiceInstanceDialog::instance() const
{
    return QJsonObject{
        {QStringLiteral("id"), m_id->text().trimmed()},
        {QStringLiteral("name"), m_name->text().trimmed()},
        {QStringLiteral("product"), serviceProductKindKey(m_product)},
        {QStringLiteral("status"), QStringLiteral("running")}
    };
}

void ServiceInstanceDialog::onAccept()
{
    if (m_id->text().trimmed().isEmpty() || m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请填写实例 ID 与名称。"));
        return;
    }
    accept();
}
