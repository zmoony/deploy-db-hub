#include "ui/tools/pages/TimestampToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
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

TimestampToolPage::TimestampToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("timestampToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *nowRow = new QWidget(this);
    auto *nowLayout = new QHBoxLayout(nowRow);
    nowLayout->setContentsMargins(0, 0, 0, 0);
    nowLayout->setSpacing(PageLayout::Space8);
    auto *nowLabel = new QLabel(nowRow);
    nowLabel->setObjectName(QStringLiteral("sectionLabel"));
    auto *copyNow = makeActionButton(QStringLiteral("复制当前毫秒"), nowRow);
    nowLayout->addWidget(nowLabel, 1);
    nowLayout->addWidget(copyNow);
    layout->addWidget(nowRow);

    auto *nowTimer = new QTimer(this);
    nowTimer->setInterval(1000);
    auto updateNow = [nowLabel]() {
        const qint64 ms = CommonTools::currentTimestampMs();
        nowLabel->setText(QStringLiteral("当前：%1 ms / %2 s / %3")
            .arg(ms)
            .arg(ms / 1000)
            .arg(CommonTools::timestampToDateText(ms, true, QString())));
    };
    connect(nowTimer, &QTimer::timeout, this, updateNow);
    nowTimer->start();
    updateNow();
    connect(copyNow, &QPushButton::clicked, this, []() {
        copyTextToClipboard(QString::number(CommonTools::currentTimestampMs()));
    });

    auto *form = new QFormLayout;
    PageLayout::applyInlineForm(form);

    auto *unit = new QComboBox(this);
    unit->addItems({QStringLiteral("毫秒"), QStringLiteral("秒")});
    PageLayout::configureFormInput(unit);
    form->addRow(QStringLiteral("时间戳单位"), unit);

    auto *format = new QLineEdit(this);
    format->setText(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    PageLayout::configureFormInput(format);
    form->addRow(QStringLiteral("时间格式"), format);

    auto *tsInput = new QLineEdit(this);
    tsInput->setText(QString::number(CommonTools::currentTimestampMs()));
    PageLayout::configureFormInput(tsInput);
    auto *toDate = makeActionButton(QStringLiteral("时间戳 → 时间"), this);
    auto *tsRow = new QWidget(this);
    auto *tsRowLayout = new QHBoxLayout(tsRow);
    tsRowLayout->setContentsMargins(0, 0, 0, 0);
    tsRowLayout->setSpacing(PageLayout::Space8);
    tsRowLayout->addWidget(tsInput, 1);
    tsRowLayout->addWidget(toDate);
    form->addRow(QStringLiteral("时间戳"), tsRow);

    auto *dateInput = new QLineEdit(this);
    dateInput->setText(CommonTools::timestampToDateText(CommonTools::currentTimestampMs(), true, QString()));
    PageLayout::configureFormInput(dateInput);
    auto *toTs = makeActionButton(QStringLiteral("时间 → 时间戳"), this);
    auto *dateRow = new QWidget(this);
    auto *dateRowLayout = new QHBoxLayout(dateRow);
    dateRowLayout->setContentsMargins(0, 0, 0, 0);
    dateRowLayout->setSpacing(PageLayout::Space8);
    dateRowLayout->addWidget(dateInput, 1);
    dateRowLayout->addWidget(toTs);
    form->addRow(QStringLiteral("时间"), dateRow);

    layout->addLayout(form);

    auto *message = new QLabel(this);
    message->setObjectName(QStringLiteral("toolMessage"));
    message->setWordWrap(true);
    layout->addWidget(message);
    layout->addStretch();

    connect(toDate, &QPushButton::clicked, this, [tsInput, unit, format, dateInput, message]() {
        bool ok = false;
        const qint64 raw = tsInput->text().trimmed().toLongLong(&ok);
        if (!ok) {
            message->setText(QStringLiteral("时间戳必须为整数"));
            return;
        }
        const bool ms = unit->currentIndex() == 0;
        const QString text = CommonTools::timestampToDateText(raw, ms, format->text());
        dateInput->setText(text);
        message->setText(QStringLiteral("已转换为时间：%1").arg(text));
    });

    connect(toTs, &QPushButton::clicked, this, [dateInput, unit, format, tsInput, message]() {
        QString error;
        const qint64 ms = CommonTools::dateTextToTimestampMs(dateInput->text(), format->text(), &error);
        if (!error.isEmpty()) {
            message->setText(error);
            return;
        }
        const bool useMs = unit->currentIndex() == 0;
        const qint64 value = useMs ? ms : ms / 1000;
        tsInput->setText(QString::number(value));
        message->setText(QStringLiteral("已转换为时间戳：%1 %2").arg(value).arg(useMs ? QStringLiteral("ms") : QStringLiteral("s")));
    });
}

QString TimestampToolPage::title() const
{
    return QStringLiteral("时间戳转换");
}

QString TimestampToolPage::subtitle() const
{
    return QStringLiteral("在 Unix 时间戳和可读日期之间转换");
}

} // namespace Tools
} // namespace Ui
