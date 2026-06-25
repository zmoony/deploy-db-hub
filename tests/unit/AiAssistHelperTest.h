#pragma once

#include <QObject>

class AiAssistHelperTest final : public QObject
{
    Q_OBJECT

private slots:
    void truncatesLongLogs();
    void sanitizesSensitiveValues();
};
