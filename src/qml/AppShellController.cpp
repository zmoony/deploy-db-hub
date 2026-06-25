#include "qml/AppShellController.h"

#include "infra/DataPaths.h"

#include <QStackedWidget>
#include <QWidget>

AppShellController::AppShellController(QObject *parent)
    : QObject(parent)
{
}

void AppShellController::initialize(const QStringList &moduleTitles,
                                  const QVector<QStringList> &navigationLabels,
                                  const QVector<QStackedWidget *> &modulePages,
                                  QStackedWidget *moduleStack,
                                  QStackedWidget *contentStack,
                                  QWidget *settingsPage)
{
    m_moduleTitles = moduleTitles;
    m_navigationLabels = navigationLabels;
    m_modulePages = modulePages;
    m_moduleStack = moduleStack;
    m_contentStack = contentStack;
    m_settingsPage = settingsPage;
    refreshNavLabels();
    refreshCurrentWidget();
}

int AppShellController::moduleIndex() const
{
    return m_moduleIndex;
}

void AppShellController::setModuleIndex(int index)
{
    if (index < 0 || index >= m_moduleTitles.size() || index == m_moduleIndex) {
        return;
    }
    showMainContent();
    m_moduleIndex = index;
    m_navIndex = 0;
    if (m_moduleStack != nullptr) {
        m_moduleStack->setCurrentIndex(index);
    }
    if (index < m_modulePages.size() && m_modulePages.at(index) != nullptr) {
        m_modulePages.at(index)->setCurrentIndex(0);
    }
    refreshNavLabels();
    refreshCurrentWidget();
    emit moduleIndexChanged();
    emit navIndexChanged();
    emit moduleActivated(index);
    emit navigationActivated(index, 0);
}

int AppShellController::navIndex() const
{
    return m_navIndex;
}

void AppShellController::setNavIndex(int index)
{
    if (index < 0 || index >= navLabels().size() || index == m_navIndex) {
        return;
    }
    showMainContent();
    m_navIndex = index;
    if (m_moduleIndex >= 0 && m_moduleIndex < m_modulePages.size()) {
        QStackedWidget *stack = m_modulePages.at(m_moduleIndex);
        if (stack != nullptr) {
            stack->setCurrentIndex(index);
        }
    }
    refreshCurrentWidget();
    emit navIndexChanged();
    emit navigationActivated(m_moduleIndex, index);
}

QStringList AppShellController::moduleTitles() const
{
    return m_moduleTitles;
}

QStringList AppShellController::navLabels() const
{
    if (m_moduleIndex < 0 || m_moduleIndex >= m_navigationLabels.size()) {
        return {};
    }
    return m_navigationLabels.at(m_moduleIndex);
}

QWidget *AppShellController::currentWidget() const
{
    return resolveCurrentWidget();
}

QString AppShellController::configDir() const
{
    return DataPaths::configDir();
}

bool AppShellController::settingsVisible() const
{
    return m_settingsVisible;
}

void AppShellController::showSettings()
{
    if (m_contentStack == nullptr || m_settingsPage == nullptr) {
        return;
    }
    m_contentStack->setCurrentIndex(1);
    m_settingsVisible = true;
    refreshCurrentWidget();
    emit settingsVisibleChanged();
    emit currentWidgetChanged();
}

void AppShellController::showMainContent()
{
    if (m_contentStack == nullptr) {
        return;
    }
    if (m_contentStack->currentIndex() != 0) {
        m_contentStack->setCurrentIndex(0);
    }
    if (m_settingsVisible) {
        m_settingsVisible = false;
        emit settingsVisibleChanged();
    }
    refreshCurrentWidget();
    emit currentWidgetChanged();
}

void AppShellController::navigateTo(int moduleIndex, int navIndex)
{
    if (moduleIndex < 0 || moduleIndex >= m_moduleTitles.size()) {
        return;
    }
    if (navIndex < 0 || navIndex >= m_navigationLabels.at(moduleIndex).size()) {
        return;
    }
    showMainContent();

    const bool moduleChanged = moduleIndex != m_moduleIndex;
    if (moduleChanged) {
        m_moduleIndex = moduleIndex;
        if (m_moduleStack != nullptr) {
            m_moduleStack->setCurrentIndex(moduleIndex);
        }
        refreshNavLabels();
        emit moduleIndexChanged();
        emit moduleActivated(moduleIndex);
    }

    if (navIndex != m_navIndex || moduleChanged) {
        m_navIndex = navIndex;
        if (moduleIndex >= 0 && moduleIndex < m_modulePages.size()) {
            QStackedWidget *stack = m_modulePages.at(moduleIndex);
            if (stack != nullptr) {
                stack->setCurrentIndex(navIndex);
            }
        }
        emit navIndexChanged();
        emit navigationActivated(moduleIndex, navIndex);
    }

    refreshCurrentWidget();
    emit currentWidgetChanged();
}

void AppShellController::refreshNavLabels()
{
    emit navLabelsChanged();
}

void AppShellController::refreshCurrentWidget()
{
    emit currentWidgetChanged();
}

QWidget *AppShellController::resolveCurrentWidget() const
{
    if (m_settingsVisible && m_settingsPage != nullptr) {
        return m_settingsPage;
    }
    if (m_moduleIndex < 0 || m_moduleIndex >= m_modulePages.size()) {
        return nullptr;
    }
    QStackedWidget *stack = m_modulePages.at(m_moduleIndex);
    if (stack == nullptr) {
        return nullptr;
    }
    return stack->widget(m_navIndex);
}
