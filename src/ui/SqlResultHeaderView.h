#pragma once

#include <QHeaderView>
#include <QSet>
#include <QStringList>

class SqlResultHeaderView final : public QHeaderView
{
    Q_OBJECT

public:
    explicit SqlResultHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr);

    void setFilterActive(int section, bool active);
    void setSortIndicator(int section, bool ascending);
    void setHeaderLabels(const QStringList &labels);
    int actionIconsWidth() const;

signals:
    void sortClicked(int section);
    void filterClicked(int section, const QPoint &globalPos);

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QRect filterIconRect(const QRect &sectionRect) const;
    QRect sortIconRect(const QRect &sectionRect) const;

    QSet<int> m_activeFilters;
    QStringList m_headerLabels;
    int m_sortSection = -1;
    bool m_sortAscending = true;
};
