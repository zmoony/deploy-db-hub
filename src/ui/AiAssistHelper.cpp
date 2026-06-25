#include "ui/AiAssistHelper.h"

#include "infra/CredentialStore.h"
#include "infra/LogSanitizer.h"

namespace AiAssistHelper {

namespace {

constexpr int kMaxLogChars = 24000;

}

bool resolveCredentials(AiSettingsStore *aiSettings,
                        CredentialStore *credentials,
                        AiSettings *settings,
                        QString *apiKey,
                        QString *error)
{
    if (aiSettings == nullptr || credentials == nullptr || settings == nullptr || apiKey == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("请先在「通用工具 → AI 配置」页完成设置");
        }
        return false;
    }

    QString loadError;
    if (!aiSettings->load(settings, &loadError)) {
        if (error != nullptr) {
            *error = loadError;
        }
        return false;
    }

    if (settings->apiBaseUrl.trimmed().isEmpty() || settings->model.trimmed().isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请先在「AI 配置」页填写 API Base URL 和模型");
        }
        return false;
    }

    *apiKey = credentials->load(settings->credentialRef);
    if (apiKey->isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请先在「AI 配置」页保存 API Key");
        }
        return false;
    }

    return true;
}

QString prepareLogForAiAnalysis(const QString &rawLog)
{
    QString sanitized = LogSanitizer::sanitize(rawLog.trimmed());
    if (sanitized.size() <= kMaxLogChars) {
        return sanitized;
    }
    return QStringLiteral("...[仅保留末尾 %1 字符]...\n%2")
        .arg(kMaxLogChars)
        .arg(sanitized.right(kMaxLogChars));
}

QString deploymentLogAnalysisSystemPrompt()
{
    return QStringLiteral(
        "You analyze deployment/build logs for a DevOps desktop tool. "
        "Reply in concise Chinese. Summarize: 1) overall result, 2) root cause if failed, "
        "3) key error lines, 4) suggested next steps. Do not invent facts not present in the log.");
}

} // namespace AiAssistHelper
