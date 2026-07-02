#include "ui/SqlColumnFilterPopup.h"

#include "ui/PageLayout.h"

#include <QCheckBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

SqlColumnFilterPopup::SqlColumnFilterPopup(QWidget *parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setObjectName(QStringLiteral("sqlColumnFilterPopup"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(PageLayout::Space12, PageLayout::Space12, PageLayout::Space12, PageLayout::Space12);
    layout->setSpacing(PageLayout::Space8);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("sqlColumnFilterTitle"));
    layout->addWidget(m_titleLabel);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName(QStringLiteral("sqlColumnFilterSearch"));
    m_searchEdit->setPlaceholderText(QStringLiteral("输入关键字筛选…"));
    m_searchEdit->setClearButtonEnabled(true);
    PageLayout::configureFormInput(m_searchEdit);
    layout->addWidget(m_searchEdit);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SqlColumnFilterPopup::onSearchChanged);

    auto *listHeader = new QWidget(this);
    auto *listHeaderLayout = new QHBoxLayout(listHeader);
    listHeaderLayout->setContentsMargins(PageLayout::Space4, 0, PageLayout::Space4, 0);
    listHeaderLayout->addWidget(new QLabel(QStringLiteral("值"), listHeader));
    listHeaderLayout->addStretch();
    listHeaderLayout->addWidget(new QLabel(QStringLiteral("计数"), listHeader));
    layout->addWidget(listHeader);

    m_valueList = new QListWidget(this);
    m_valueList->setObjectName(QStringLiteral("sqlColumnFilterList"));
    m_valueList->setMinimumHeight(160);
    m_valueList->setMaximumHeight(240);
    m_valueList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_valueList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    layout->addWidget(m_valueList, 1);

    auto *actions = new QHBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    actions->addStretch();
    auto *clearButton = new QPushButton(QStringLiteral("清除筛选"), this);
    clearButton->setObjectName(QStringLiteral("toolSecondaryButton"));
    clearButton->setFixedHeight(28);
    connect(clearButton, &QPushButton::clicked, this, &SqlColumnFilterPopup::onClear);
    actions->addWidget(clearButton);
    layout->addLayout(actions);

    setMinimumWidth(280);
    setMaximumWidth(360);
}

void SqlColumnFilterPopup::openForColumn(int column,
                                         const QString &columnName,
                                         const QVector<QPair<QString, int>> &valueCounts,
                                         const SqlColumnFilterState &state,
                                         const QPoint &globalAnchor)
{
    m_column = column;
    m_columnName = columnName;
    m_allValueCounts = valueCounts;
    m_valueChecks.clear();
    m_valueList->clear();

    m_titleLabel->setText(QStringLiteral("「%1」本地筛选").arg(columnName));
    m_searchEdit->blockSignals(true);
    m_searchEdit->setText(state.searchText);
    m_searchEdit->blockSignals(false);

    for (const QPair<QString, int> &entry : m_allValueCounts) {
        auto *item = new QListWidgetItem(m_valueList);
        auto *row = new QWidget(m_valueList);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(PageLayout::Space4, 2, PageLayout::Space4, 2);
        rowLayout->setSpacing(PageLayout::Space8);

        auto *check = new QCheckBox(entry.first, row);
        check->setChecked(!state.excludedValues.contains(entry.first));
        connect(check, &QCheckBox::toggled, this, &SqlColumnFilterPopup::onValueToggled);
        m_valueChecks.insert(entry.first, check);

        auto *countLabel = new QLabel(QString::number(entry.second), row);
        countLabel->setObjectName(QStringLiteral("mutedText"));
        countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        rowLayout->addWidget(check, 1);
        rowLayout->addWidget(countLabel);
        item->setSizeHint(row->sizeHint());
        m_valueList->setItemWidget(item, row);
    }

    rebuildValueList();

    adjustSize();
    const QSize popupSize = sizeHint().expandedTo(QSize(280, 280));
    QPoint pos = globalAnchor;
    pos.setY(pos.y() + 4);
    if (QScreen *screen = QGuiApplication::screenAt(pos)) {
        const QRect avail = screen->availableGeometry();
        if (pos.x() + popupSize.width() > avail.right()) {
            pos.setX(avail.right() - popupSize.width());
        }
        if (pos.y() + popupSize.height() > avail.bottom()) {
            pos.setY(globalAnchor.y() - popupSize.height() - 4);
        }
    }
    setGeometry(QRect(pos, popupSize));
    show();
    m_searchEdit->setFocus();
}

void SqlColumnFilterPopup::rebuildValueList()
{
    const QString keyword = m_searchEdit->text().trimmed();
    for (int row = 0; row < m_valueList->count(); ++row) {
        QListWidgetItem *item = m_valueList->item(row);
        QWidget *widget = m_valueList->itemWidget(item);
        auto *check = widget != nullptr ? widget->findChild<QCheckBox *>() : nullptr;
        if (check == nullptr) {
            continue;
        }
        const bool visible = keyword.isEmpty() || check->text().contains(keyword, Qt::CaseInsensitive);
        item->setHidden(!visible);
    }
}

void SqlColumnFilterPopup::onSearchChanged(const QString &text)
{
    Q_UNUSED(text);
    rebuildValueList();
    applyCurrentState();
}

void SqlColumnFilterPopup::onValueToggled()
{
    applyCurrentState();
}

void SqlColumnFilterPopup::onClear()
{
    if (m_column < 0) {
        hide();
        return;
    }
    emit filterCleared(m_column);
    hide();
}

SqlColumnFilterState SqlColumnFilterPopup::currentState() const
{
    SqlColumnFilterState state;
    state.searchText = m_searchEdit->text().trimmed();
    for (auto it = m_valueChecks.constBegin(); it != m_valueChecks.constEnd(); ++it) {
        if (!it.value()->isChecked()) {
            state.excludedValues.insert(it.key());
        }
    }
    return state;
}

void SqlColumnFilterPopup::applyCurrentState()
{
    if (m_column < 0) {
        return;
    }
    emit filterApplied(m_column, currentState());
}
