#pragma once

#include <QObject>
#include <QStringList>

class QButtonGroup;
class QStackedWidget;

class LineTabBarController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList labels READ labels CONSTANT)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    explicit LineTabBarController(QObject *parent = nullptr);

    QStringList labels() const;
    int currentIndex() const;
    void setLabels(const QStringList &labels);
    void setCurrentIndex(int index);
    void bindStack(QStackedWidget *stack);
    void bindButtonGroup(QButtonGroup *group);

signals:
    void currentIndexChanged();
    void tabActivated(int index);

private:
    QStringList m_labels;
    QStackedWidget *m_stack = nullptr;
    QButtonGroup *m_group = nullptr;
    int m_currentIndex = -1;
};
