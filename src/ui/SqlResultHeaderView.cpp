#include "ui/SqlResultHeaderView.h"

#include <QMouseEvent>
#include <QPainter>
#include <QFontMetrics>

namespace {

constexpr int kFilterIconWidth = 16;
constexpr int kSortIconWidth = 16;
constexpr int kIconRightPadding = 6;
constexpr int kIconGap = 4;
constexpr int kIconHeight = 16;

void paintFilterIcon(QPainter &painter, const QRect &rect, const QColor &color)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);

    const int cx = rect.center().x();
    const int top = rect.top() + 3;
    const int mid = rect.center().y() + 1;
    const int bottom = rect.bottom() - 2;
    const int halfTop = 5;
    const int halfNeck = 2;

    QPolygonF funnel;
    funnel << QPointF(cx - halfTop, top) << QPointF(cx + halfTop, top) << QPointF(cx + halfNeck, mid)
           << QPointF(cx + halfNeck, bottom) << QPointF(cx - halfNeck, bottom) << QPointF(cx - halfNeck, mid);
    painter.drawPolygon(funnel);
    painter.restore();
}

void paintSortIcon(QPainter &painter, const QRect &rect, bool sorted, bool ascending)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    const QColor active(QStringLiteral("#2563EB"));
    const QColor inactive(QStringLiteral("#94A3B8"));
    const int cx = rect.center().x();
    const int midY = rect.center().y();

    const auto drawTriangle = [&](bool up, const QColor &color) {
        painter.setBrush(color);
        QPolygonF triangle;
        if (up) {
            triangle << QPointF(cx, rect.top() + 2) << QPointF(cx - 4, midY - 1) << QPointF(cx + 4, midY - 1);
        } else {
            triangle << QPointF(cx, rect.bottom() - 2) << QPointF(cx - 4, midY + 1) << QPointF(cx + 4, midY + 1);
        }
        painter.drawPolygon(triangle);
    };

    if (!sorted) {
        drawTriangle(true, inactive);
        drawTriangle(false, inactive);
    } else if (ascending) {
        drawTriangle(true, active);
        drawTriangle(false, inactive);
    } else {
        drawTriangle(true, inactive);
        drawTriangle(false, active);
    }

    painter.restore();
}

} // namespace

SqlResultHeaderView::SqlResultHeaderView(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent)
{
    setObjectName(QStringLiteral("sqlResultHeader"));
    setSectionsClickable(false);
    setHighlightSections(false);
    setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setMinimumSectionSize(72);
    setStretchLastSection(false);
    setCascadingSectionResizes(false);
}

void SqlResultHeaderView::setFilterActive(int section, bool active)
{
    if (active) {
        m_activeFilters.insert(section);
    } else {
        m_activeFilters.remove(section);
    }
    updateSection(section);
}

void SqlResultHeaderView::setSortIndicator(int section, bool ascending)
{
    m_sortSection = section;
    m_sortAscending = ascending;
    viewport()->update();
}

void SqlResultHeaderView::setHeaderLabels(const QStringList &labels)
{
    m_headerLabels = labels;
    viewport()->update();
}

QRect SqlResultHeaderView::filterIconRect(const QRect &sectionRect) const
{
    return QRect(sectionRect.right() - kFilterIconWidth - kIconRightPadding,
                 sectionRect.top() + (sectionRect.height() - kIconHeight) / 2,
                 kFilterIconWidth,
                 kIconHeight);
}

QRect SqlResultHeaderView::sortIconRect(const QRect &sectionRect) const
{
    const QRect filterRect = filterIconRect(sectionRect);
    return QRect(filterRect.left() - kIconGap - kSortIconWidth,
                 filterRect.top(),
                 kSortIconWidth,
                 kIconHeight);
}

int SqlResultHeaderView::actionIconsWidth() const
{
    return kFilterIconWidth + kSortIconWidth + kIconGap + kIconRightPadding + 8;
}

void SqlResultHeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    if (!rect.isValid()) {
        return;
    }

    painter->save();
    painter->setClipRect(rect);
    painter->fillRect(rect, palette().color(QPalette::Base));

    const bool sorted = logicalIndex == m_sortSection;
    const bool filtered = m_activeFilters.contains(logicalIndex);
    const QRect filterRect = filterIconRect(rect);
    const QRect sortRect = sortIconRect(rect);
    const int textRightInset = qMax(0, rect.right() - sortRect.left() + 4);
    const QRect textRect = rect.adjusted(8, 0, -textRightInset, 0);

    QString label;
    if (logicalIndex >= 0 && logicalIndex < m_headerLabels.size()) {
        label = m_headerLabels.at(logicalIndex);
    } else if (model() != nullptr) {
        label = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
    }

    painter->setPen(palette().color(QPalette::Text));
    QFont font = painter->font();
    font.setBold(sorted);
    painter->setFont(font);
    const QString elidedLabel =
        textRect.width() > 0
            ? painter->fontMetrics().elidedText(label, Qt::ElideRight, textRect.width())
            : QString();
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedLabel);

    paintSortIcon(*painter, sortRect, sorted, m_sortAscending);
    paintFilterIcon(*painter,
                    filterRect,
                    filtered ? QColor(QStringLiteral("#2563EB")) : palette().color(QPalette::Mid));

    painter->setPen(QColor(QStringLiteral("#E5E7EB")));
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    painter->restore();
}

void SqlResultHeaderView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const int section = logicalIndexAt(event->pos());
        if (section >= 0) {
            const QRect sectionRect(sectionViewportPosition(section), 0, sectionSize(section), height());
            if (filterIconRect(sectionRect).contains(event->pos())) {
                emit filterClicked(section, mapToGlobal(event->pos()));
                event->accept();
                return;
            }
            if (sortIconRect(sectionRect).contains(event->pos())) {
                emit sortClicked(section);
                event->accept();
                return;
            }
        }
    }

    QHeaderView::mousePressEvent(event);
}
