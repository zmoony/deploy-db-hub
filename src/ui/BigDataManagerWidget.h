#pragma once

#include <QStringList>
#include <QWidget>

class QStackedWidget;

class BigDataManagerWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit BigDataManagerWidget(class ConfigStore *store,
                                  class CredentialStore *credentials,
                                  class CredentialSessionCache *sessionCache,
                                  QWidget *parent = nullptr);
    QStringList sectionLabels() const;
    QWidget *takeSectionPage(int index);
    int sectionPageCount() const;

private:
    QStackedWidget *m_stack = nullptr;
};
