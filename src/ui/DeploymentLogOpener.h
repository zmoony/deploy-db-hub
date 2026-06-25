#pragma once

#include <QJsonObject>

class AiSettingsStore;
class CredentialSessionCache;
class CredentialStore;
class QWidget;

namespace DeploymentLogOpener {

void open(QWidget *parent,
          CredentialStore *credentials,
          CredentialSessionCache *sessionCache,
          const QJsonObject &server,
          const QString &logPath,
          AiSettingsStore *aiSettings = nullptr);

QString resolveLogPathForProject(const QJsonObject &project, QWidget *parent, bool *ok);

}
