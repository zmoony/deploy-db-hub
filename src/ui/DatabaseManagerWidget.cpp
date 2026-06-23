#include "ui/DatabaseManagerWidget.h"

#include "infra/ConfigStore.h"
#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"
#include "ui/ServiceNodeTypes.h"
#include "ui/ServiceProductPanel.h"

#include <QStackedWidget>
#include <QVBoxLayout>

DatabaseManagerWidget::DatabaseManagerWidget(ConfigStore *store,
                                             CredentialStore *credentials,
                                             CredentialSessionCache *sessionCache,
                                             QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(new ServiceProductPanel(ServiceProductKind::Oracle, store, credentials, sessionCache, this));
    m_stack->addWidget(new ServiceProductPanel(ServiceProductKind::PostgreSQL, store, credentials, sessionCache, this));
    layout->addWidget(m_stack, 1);
}

QStringList DatabaseManagerWidget::sectionLabels() const
{
    return {QStringLiteral("Oracle"), QStringLiteral("PostgreSQL")};
}

QWidget *DatabaseManagerWidget::takeSectionPage(int index)
{
    if (m_stack == nullptr || index < 0 || index >= m_stack->count()) {
        return nullptr;
    }
    QWidget *page = m_stack->widget(index);
    m_stack->removeWidget(page);
    page->setParent(nullptr);
    return page;
}

int DatabaseManagerWidget::sectionPageCount() const
{
    return m_stack != nullptr ? m_stack->count() : 0;
}
