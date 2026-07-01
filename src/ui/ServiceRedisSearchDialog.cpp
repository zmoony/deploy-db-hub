#include "ui/ServiceRedisSearchDialog.h"

#include "ui/PageLayout.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

ServiceRedisSearchDialog::ServiceRedisSearchDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Key 搜索"));
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("dialogFormPanel"));
    card->setAttribute(Qt::WA_StyledBackground, true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    auto *header = new QWidget(card);
    header->setObjectName(QStringLiteral("dialogFormPanelHeader"));
    header->setAttribute(Qt::WA_StyledBackground, true);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space12, PageLayout::Space16, PageLayout::Space12);
    headerLayout->setSpacing(0);
    auto *headerTitle = new QLabel(QStringLiteral("搜索条件"), header);
    headerTitle->setObjectName(QStringLiteral("dialogFormPanelTitle"));
    headerLayout->addWidget(headerTitle);
    headerLayout->addStretch();
    cardLayout->addWidget(header);

    auto *body = new QWidget(card);
    auto *form = new QFormLayout(body);
    form->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    form->setVerticalSpacing(PageLayout::Space12);
    form->setHorizontalSpacing(PageLayout::Space16);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_patternEdit = new QLineEdit(body);
    m_patternEdit->setText(QStringLiteral("*"));
    m_patternEdit->setPlaceholderText(QStringLiteral("匹配模式，支持通配符 * 与 ?，例如 user:*:profile"));
    m_patternEdit->setClearButtonEnabled(true);
    PageLayout::configureFormInput(m_patternEdit);
    form->addRow(QStringLiteral("匹配模式"), m_patternEdit);

    m_typeCombo = new QComboBox(body);
    m_typeCombo->addItem(QStringLiteral("全部类型"), QString());
    m_typeCombo->addItem(QStringLiteral("string 字符串"), QStringLiteral("string"));
    m_typeCombo->addItem(QStringLiteral("list 列表"), QStringLiteral("list"));
    m_typeCombo->addItem(QStringLiteral("hash 哈希"), QStringLiteral("hash"));
    m_typeCombo->addItem(QStringLiteral("set 集合"), QStringLiteral("set"));
    m_typeCombo->addItem(QStringLiteral("zset 有序集合"), QStringLiteral("zset"));
    m_typeCombo->addItem(QStringLiteral("stream 流"), QStringLiteral("stream"));
    PageLayout::configureFormInput(m_typeCombo);
    form->addRow(QStringLiteral("类型筛选"), m_typeCombo);

    cardLayout->addWidget(body);
    layout->addWidget(card);

    QLabel *hintText = nullptr;
    layout->addWidget(PageLayout::wrapDialogHintSection(QString(), this, &hintText));
    if (hintText != nullptr) {
        hintText->setText(QStringLiteral(
            "匹配模式使用 Redis 通配符（* 任意字符，? 单个字符）。\n"
            "类型筛选留空表示不过滤；非 string 类型的大小返回元素数。"));
    }

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("搜索"));
    buttonBox->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("primaryButton"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    buttonBox->button(QDialogButtonBox::Ok)->setMinimumHeight(PageLayout::DialogFieldHeight);
    buttonBox->button(QDialogButtonBox::Cancel)->setMinimumHeight(PageLayout::DialogFieldHeight);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    m_patternEdit->setFocus();

    PageLayout::applyModalDialog(this);
    resize(440, 280);
}

QString ServiceRedisSearchDialog::pattern() const
{
    const QString text = m_patternEdit->text().trimmed();
    return text.isEmpty() ? QStringLiteral("*") : text;
}

QString ServiceRedisSearchDialog::typeFilter() const
{
    return m_typeCombo->currentData().toString();
}

QString ServiceRedisSearchDialog::typeFilterAll()
{
    return QString();
}
