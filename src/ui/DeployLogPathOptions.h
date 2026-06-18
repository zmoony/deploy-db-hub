#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

QStringList deployLogPathOptionsForProject(const QJsonObject &project);
bool isRemoteDeployLogPath(const QString &path);
