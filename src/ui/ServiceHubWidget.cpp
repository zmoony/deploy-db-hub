#include "ui/ServiceHubWidget.h"

#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"
#include "ui/BigDataManagerWidget.h"
#include "ui/DatabaseManagerWidget.h"
#include "ui/PageLayout.h"
#include "ui/ServerManagerWidget.h"

#include <QButtonGroup>
#include <QStackedWidget>
#include <QVBoxLayout>

ServiceHubWidget::ServiceHubWidget(ConfigStore *store,
                                   CredentialStore *credentials,
                                   CredentialSessionCache *sessionCache,
                                   QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyPage(layout);
    layout->setSpacing(PageLayout::Space12);

    layout->addWidget(PageLayout::makeHeaderBlock(
        QStringLiteral("服务管理"),
        QStringLiteral("统一管理服务器、大数据组件与数据库连接，支持配置、运维与内容查看。"),
        this));

    auto *stack = new QStackedWidget(this);
    m_serverManager = new ServerManagerWidget(store, this, false);
    m_bigDataManager = new BigDataManagerWidget(store, credentials, sessionCache, this);
    m_databaseManager = new DatabaseManagerWidget(store, credentials, sessionCache, this);
    stack->addWidget(m_serverManager);
    stack->addWidget(m_bigDataManager);
    stack->addWidget(m_databaseManager);

    QButtonGroup *tabGroup = nullptr;
    layout->addWidget(PageLayout::makeTabBar(
        {
            QStringLiteral("服务器"),
            QStringLiteral("大数据"),
            QStringLiteral("数据库")
        },
        this,
        &tabGroup,
        stack));
    layout->addWidget(stack, 1);

    connect(m_serverManager, &ServerManagerWidget::serversChanged, this, &ServiceHubWidget::serversChanged);
}

ServerManagerWidget *ServiceHubWidget::serverManager() const
{
    return m_serverManager;
}

int ServiceHubWidget::serverCount() const
{
    return m_serverManager != nullptr ? m_serverManager->serverCount() : 0;
}

QStringList ServiceHubWidget::serverIds() const
{
    return m_serverManager != nullptr ? m_serverManager->serverIds() : QStringList{};
}
