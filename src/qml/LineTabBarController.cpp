#include "qml/LineTabBarController.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QStackedWidget>

LineTabBarController::LineTabBarController(QObject *parent)
    : QObject(parent)
{
}

QStringList LineTabBarController::labels() const
{
    return m_labels;
}

int LineTabBarController::currentIndex() const
{
    return m_currentIndex;
}

void LineTabBarController::setLabels(const QStringList &labels)
{
    m_labels = labels;
}

void LineTabBarController::setCurrentIndex(int index)
{
    if (index < 0 || index >= m_labels.size() || index == m_currentIndex) {
        return;
    }
    m_currentIndex = index;
    if (m_group != nullptr) {
        if (QAbstractButton *button = m_group->button(index)) {
            button->setChecked(true);
        }
    }
    if (m_stack != nullptr) {
        m_stack->setCurrentIndex(index);
    }
    emit currentIndexChanged();
    emit tabActivated(index);
}

void LineTabBarController::bindStack(QStackedWidget *stack)
{
    m_stack = stack;
    if (m_stack != nullptr && m_currentIndex >= 0 && m_currentIndex < m_stack->count()) {
        m_stack->setCurrentIndex(m_currentIndex);
    }
}

void LineTabBarController::bindButtonGroup(QButtonGroup *group)
{
    if (m_group == group) {
        return;
    }
    m_group = group;
    if (m_group == nullptr) {
        return;
    }
    connect(m_group, &QButtonGroup::idClicked, this, [this](int id) {
        if (id < 0 || id >= m_labels.size() || id == m_currentIndex) {
            return;
        }
        m_currentIndex = id;
        emit currentIndexChanged();
        emit tabActivated(id);
    });
}
