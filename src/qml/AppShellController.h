#pragma once

#include <QObject>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QStackedWidget;

class AppShellController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int moduleIndex READ moduleIndex WRITE setModuleIndex NOTIFY moduleIndexChanged)
    Q_PROPERTY(int navIndex READ navIndex WRITE setNavIndex NOTIFY navIndexChanged)
    Q_PROPERTY(QStringList moduleTitles READ moduleTitles CONSTANT)
    Q_PROPERTY(QStringList navLabels READ navLabels NOTIFY navLabelsChanged)
    Q_PROPERTY(QWidget *currentWidget READ currentWidget NOTIFY currentWidgetChanged)
    Q_PROPERTY(QString configDir READ configDir CONSTANT)
    Q_PROPERTY(bool settingsVisible READ settingsVisible NOTIFY settingsVisibleChanged)

public:
    explicit AppShellController(QObject *parent = nullptr);

    void initialize(const QStringList &moduleTitles,
                    const QVector<QStringList> &navigationLabels,
                    const QVector<QStackedWidget *> &modulePages,
                    QStackedWidget *moduleStack,
                    QStackedWidget *contentStack,
                    QWidget *settingsPage);

    int moduleIndex() const;
    void setModuleIndex(int index);

    int navIndex() const;
    void setNavIndex(int index);

    QStringList moduleTitles() const;
    QStringList navLabels() const;
    QWidget *currentWidget() const;
    QString configDir() const;
    bool settingsVisible() const;

    Q_INVOKABLE void showSettings();
    Q_INVOKABLE void showMainContent();
    Q_INVOKABLE void navigateTo(int moduleIndex, int navIndex);

signals:
    void moduleIndexChanged();
    void navIndexChanged();
    void navLabelsChanged();
    void currentWidgetChanged();
    void settingsVisibleChanged();
    void moduleActivated(int index);
    void navigationActivated(int moduleIndex, int navIndex);

private:
    void refreshNavLabels();
    void refreshCurrentWidget();
    QWidget *resolveCurrentWidget() const;

    QStringList m_moduleTitles;
    QVector<QStringList> m_navigationLabels;
    QVector<QStackedWidget *> m_modulePages;
    QStackedWidget *m_moduleStack = nullptr;
    QStackedWidget *m_contentStack = nullptr;
    QWidget *m_settingsPage = nullptr;
    int m_moduleIndex = 0;
    int m_navIndex = 0;
    bool m_settingsVisible = false;
};
