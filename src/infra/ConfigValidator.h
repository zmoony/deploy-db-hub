#pragma once

#include <QJsonObject>
#include <QStringList>

struct ValidationResult {
    bool ok = false;
    QStringList errors;
};

class ConfigValidator final
{
public:
    static ValidationResult validateProject(const QJsonObject &project);
    static ValidationResult validateServer(const QJsonObject &server);
    static ValidationResult validateDeployment(const QJsonObject &record);
};
