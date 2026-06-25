#pragma once

#include <QQuickItem>
#include <QWidget>

class WidgetHostItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QWidget *widget READ widget WRITE setWidget NOTIFY widgetChanged)

public:
    explicit WidgetHostItem(QQuickItem *parent = nullptr);

    QWidget *widget() const;
    void setWidget(QWidget *widget);

signals:
    void widgetChanged();

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void updateContainer();
    void updateContainerGeometry();

    QWidget *m_widget = nullptr;
    QWidget *m_container = nullptr;
};
