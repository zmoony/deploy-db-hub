#include "infra/LogSanitizer.h"

#include <QRegularExpression>

QString LogSanitizer::sanitize(const QString &text)
{
    QString output = text;

    const QStringList sensitiveKeys = {
        QStringLiteral("password"),
        QStringLiteral("passwd"),
        QStringLiteral("secret"),
        QStringLiteral("token"),
        QStringLiteral("apikey"),
        QStringLiteral("api_key"),
        QStringLiteral("authorization")
    };

    for (const QString &key : sensitiveKeys) {
        const QRegularExpression pattern(QStringLiteral("(%1\\s*[:=]\\s*)([^\\s&]+)").arg(QRegularExpression::escape(key)),
            QRegularExpression::CaseInsensitiveOption);
        output.replace(pattern, QStringLiteral("\\1***"));
    }

    output.replace(QRegularExpression(QStringLiteral("(Authorization:\\s*)\\S+.*"), QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("\\1***"));
    output.replace(QRegularExpression(QStringLiteral("([?&](?:token|access_token)=)[^&\\s]+"), QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("\\1***"));
    output.replace(QRegularExpression(QStringLiteral("-----BEGIN [^-]*PRIVATE KEY-----[\\s\\S]*?-----END [^-]*PRIVATE KEY-----")),
        QStringLiteral("***"));

    return output;
}
